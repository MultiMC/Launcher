#!/usr/bin/env python3
"""Make updater v2's test trees in testdata/trees, for the unit tests next to this script.

    launcher/updater/v2/make-test-trees.py <hopper checkout>

Every tree is written and signed with hopper's format code (lib/updater_formats.py, the
validator the publisher uses), so the tests read what the publisher writes. The good trees
must pass the validator's whole-tree check, the broken one must fail it. The keys are
throwaway Ed25519 keys made with openssl in a temporary directory; the private keys are not
kept, so running this again makes new keys and new signatures (and the tests' expectations
of public keys come from the trees, not from the tests). The test time is 2026-11-01.

Each tree is in `<name>/updater/`, so `<name>/` stands for the files site the tree is on, as a
mirror list names it.

Trees (all for keys/1.json of `main` as the floor, pinned root keys root1 and root2):

main     keys 1 (online1), keys 2 (online key rotated to online2); builds 10, 11, 12;
         tags serial 7: stable 10, testing 11, latest 12, build 9 withdrawn
stale    keys 1 only; build 10; tags serial 3, everything on 10 (a mirror that lags behind)
broken   main with tags.json changed after it was signed
rotated  root keys rotated: keys 2 lists root3 and root4 (signed by root1 and root2), keys 3
         (signed by root3) has online3; build 10; tags serial 8
"""

import argparse
import datetime
import hashlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

NOW = datetime.datetime(2026, 11, 1, tzinfo=datetime.timezone.utc)
OUT = Path(__file__).resolve().parent / "testdata" / "trees"

# .invalid never resolves (RFC 2606): the tests try these first and fail quickly, without a network
MIRRORS = [["https://files.invalid/"], ["https://r2.invalid/", "https://mirror.invalid/"]]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("hopper", type=Path, help="a hopper checkout, for lib/updater_formats.py")
    args = parser.parse_args()
    sys.path.insert(0, str(args.hopper.resolve() / "lib"))
    global formats
    import updater_formats as formats

    with tempfile.TemporaryDirectory() as tmp:
        keys = {}
        for name in ("root1", "root2", "root3", "root4", "online1", "online2", "online3"):
            pem = Path(tmp) / (name + ".pem")
            subprocess.run(["openssl", "genpkey", "-algorithm", "ed25519", "-out", str(pem)], check=True)
            keys[name] = (pem, formats.public_key(pem))

        if OUT.exists():
            shutil.rmtree(OUT)
        make_main(tree("main"), keys)
        make_stale(tree("stale"), keys)
        make_broken(tree("broken"), tree("main"))
        make_rotated(tree("rotated"), keys)

        floor = (tree("main") / "keys" / "1.json").read_bytes()
        for name in ("main", "stale", "rotated"):
            summary, warnings = formats.check_tree(tree(name), floor, now=NOW)
            print("%s: %s %s" % (name, summary, warnings))
        try:
            formats.check_tree(tree("broken"), floor, now=NOW)
            sys.exit("broken: the validator accepts it")
        except formats.Invalid as e:
            print("broken: refused as intended: %s" % e)


def tree(name):
    return OUT / name / "updater"


def write(root, name, data):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def document(root, name, doc, *signers):
    """Write a document checked by the validator, and its signature by each signer."""
    data = formats.dump_json(doc)
    formats.check_document(name, data)
    write(root, name, data)
    write(root, name + ".sig", "".join(formats.sign(data, pem) for pem, _ in signers).encode("ascii"))


def keys_doc(root, version, expires, roots, online, *signers):
    document(root, "keys/%d.json" % version, {
        "type": "keys", "format": 1, "version": version, "expires": expires,
        "root_keys": [pub for _, pub in roots], "online_keys": [pub for _, pub in online],
    }, *signers)


def content(text):
    # text ending in a newline, without @ (the test data is copied with configure_file)
    return (text + "\n").encode("ascii")


LINUX = {"os": "linux", "arch": "x86_64", "glibc": "2.19", "glibcxx": "3.4.21", "qt": "5.6"}
WINDOWS = {"os": "windows", "arch": "x86", "os_version": "5.1"}
MAC_OLD = {"os": "macos", "arch": "x86_64", "os_version": "10.7"}
MAC_NEW = {"os": "macos", "arch": "x86_64", "os_version": "10.13"}
FRAMEWORK = "MultiMC.app/Contents/Frameworks/QtCore.framework/"
MAC_LINKS = [
    {"path": FRAMEWORK + "QtCore", "target": "Versions/Current/QtCore"},
    {"path": FRAMEWORK + "Versions/Current", "target": "5"},
]


def variant(root, name, requires, files, links=(), supersedes=()):
    """files: {path: (text, executable)}; writes the content files."""
    entries = []
    for path in sorted(files):
        text, executable = files[path]
        data = content(text)
        sha256 = hashlib.sha256(data).hexdigest()
        write(root, formats.content_path(sha256), data)
        entries.append({"path": path, "sha256": sha256, "size": len(data), "executable": executable})
    return {"name": name, "supersedes": list(supersedes), "requires": requires, "files": entries, "links": list(links)}


def build_doc(root, build, variants, signer):
    document(root, "builds/%d.json" % build, {
        "type": "build", "format": 1, "build": build, "commit": "%040x" % (0xc0ffee + build),
        "built": "2026-10-%02dT12:00:00Z" % (build % 28 + 1), "variants": variants,
    }, signer)


def linux_files(build):
    files = {
        "MultiMC": ("launcher script, build %d" % build, True),
        "bin/MultiMC": ("launcher binary, build %d" % build, True),
        "bin/libfoo.so": ("a library that never changes", False),
    }
    files["share/old.txt" if build == 10 else "share/new.txt"] = ("data file", False)
    return files


def mac_files(build, qt):
    return {
        "MultiMC.app/Contents/MacOS/MultiMC": ("mac binary %s, build %d" % (qt, build), True),
        FRAMEWORK + "Versions/5/QtCore": ("QtCore %s" % qt, False),
    }


def windows_files(build):
    return {"MultiMC.exe": ("windows binary, build %d" % build, False), "jars/NewLaunch.jar": ("jar", False)}


def builds(root, numbers, signer):
    for build in numbers:
        variants = {
            "linux-x86_64-qt5": variant(root, "Linux, 64-bit", dict(LINUX, glibc="2.27") if build == 12 else LINUX, linux_files(build)),
            "windows-x86-qt5.6": variant(root, "Windows, Qt 5.6", WINDOWS, windows_files(build)),
            "macos-x86_64-qt5.6": variant(root, "macOS, Qt 5.6", MAC_OLD, mac_files(build, "5.6"), MAC_LINKS),
        }
        if build >= 11:
            variants["macos-x86_64-qt5.15"] = variant(root, "macOS, Qt 5.15", MAC_NEW, mac_files(build, "5.15"), MAC_LINKS, ["macos-x86_64-qt5.6"])
        build_doc(root, build, variants, signer)


def tags(root, serial, latest, testing, stable, withdrawn, signer):
    document(root, "tags.json", {
        "type": "tags", "format": 1, "serial": serial, "issued": "2026-10-31T12:00:00Z",
        "tags": {"latest": latest, "testing": testing, "stable": stable}, "withdrawn": withdrawn,
    }, signer)


def mirrors(root, issued, signer):
    document(root, "mirrors.json", {"type": "mirrors", "format": 1, "issued": issued, "mirrors": MIRRORS}, signer)


def make_main(root, k):
    keys_doc(root, 1, "2027-01-01T00:00:00Z", [k["root1"], k["root2"]], [k["online1"]], k["root1"], k["root2"])
    keys_doc(root, 2, "2028-11-01T00:00:00Z", [k["root1"], k["root2"]], [k["online2"]], k["root1"])
    builds(root, [10, 11, 12], k["online2"])
    mirrors(root, "2026-10-30T00:00:00Z", k["online2"])
    tags(root, 7, 12, 11, 10, [9], k["online2"])


def make_stale(root, k):
    keys_doc(root, 1, "2027-01-01T00:00:00Z", [k["root1"], k["root2"]], [k["online1"]], k["root1"], k["root2"])
    builds(root, [10], k["online1"])
    mirrors(root, "2026-10-01T00:00:00Z", k["online1"])
    tags(root, 3, 10, 10, 10, [], k["online1"])


def make_broken(root, main_tree):
    shutil.copytree(main_tree, root)
    data = (root / "tags.json").read_bytes()
    (root / "tags.json").write_bytes(data.replace(b'"serial": 7', b'"serial": 9'))


def make_rotated(root, k):
    keys_doc(root, 1, "2027-01-01T00:00:00Z", [k["root1"], k["root2"]], [k["online1"]], k["root1"], k["root2"])
    keys_doc(root, 2, "2027-06-01T00:00:00Z", [k["root3"], k["root4"]], [k["online1"]], k["root1"], k["root2"])
    keys_doc(root, 3, "2028-11-01T00:00:00Z", [k["root3"], k["root4"]], [k["online3"]], k["root3"])
    builds(root, [10], k["online3"])
    mirrors(root, "2026-10-30T00:00:00Z", k["online3"])
    tags(root, 8, 10, 10, 10, [], k["online3"])


if __name__ == "__main__":
    main()
