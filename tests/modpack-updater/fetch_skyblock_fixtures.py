#!/usr/bin/env python3
"""Download public test releases into a disposable cache. Never opens Minecraft.

Requires Python 3 and curl. Mod files (~300 MB) require --download-files.
Use --old/--new to pin exact Modrinth version IDs for reproducible tests.
"""
import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import re
import subprocess
import zipfile


def fetch(url, destination=None):
    if not url.startswith("https://"):
        raise ValueError("Only HTTPS fixture downloads are supported")
    args = ["curl", "--fail", "--silent", "--show-error", "--location",
            "--retry", "2", "--max-time", "120", "--proto", "=https",
            "--proto-redir", "=https", "-A", "MultiMC-PackUpdater-Tests/1.0"]
    if destination:
        subprocess.run(args + ["-o", str(destination), url], check=True)
    else:
        return subprocess.check_output(args + [url])


def release_file(version):
    files = [f for f in version["files"] if f["filename"].endswith(".mrpack")]
    return next((f for f in files if f["primary"]), files[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--project", default="e0oMrxjp")
    parser.add_argument("--old")
    parser.add_argument("--new")
    parser.add_argument("--download-files", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]+", args.project):
        parser.error("Invalid project ID")
    if bool(args.old) != bool(args.new):
        parser.error("Provide both --old and --new")
    versions = json.loads(fetch("https://api.modrinth.com/v2/project/" + args.project + "/version"))
    releases = sorted((v for v in versions if v["version_type"] == "release"), key=lambda v: v["date_published"], reverse=True)
    pair = None
    if args.old:
        pair = [next(v for v in versions if v["id"] == key) for key in (args.old, args.new)]
    else:
        for latest in releases:
            previous = next((v for v in releases if v["date_published"] < latest["date_published"]
                             and set(v["game_versions"]) & set(latest["game_versions"])
                             and set(v["loaders"]) & set(latest["loaders"])
                             and release_file(v)["hashes"]["sha512"] != release_file(latest)["hashes"]["sha512"]), None)
            if previous:
                pair = [previous, latest]
                break
    if not pair:
        raise RuntimeError("No pair of distinct compatible stable releases")
    args.output.mkdir(parents=True, exist_ok=True)
    files = {}
    for label, version in zip(("old", "new"), pair):
        f = release_file(version)
        content = fetch(f["url"])
        if hashlib.sha512(content).hexdigest() != f["hashes"]["sha512"]:
            raise ValueError("Archive checksum mismatch")
        archive = args.output / (label + ".mrpack")
        archive.write_bytes(content)
        (args.output / (label + ".json")).write_text(json.dumps(version), encoding="utf-8")
        with zipfile.ZipFile(archive) as pack:
            manifest = json.loads(pack.read("modrinth.index.json"))
        for entry in manifest["files"]:
            if entry.get("env", {}).get("client") == "unsupported":
                continue
            algo = next(a for a in ("sha512", "sha256", "sha1") if a in entry["hashes"])
            digest = entry["hashes"][algo]
            if not re.fullmatch(r"[0-9a-fA-F]{" + str(hashlib.new(algo).digest_size * 2) + r"}", digest):
                raise ValueError("Invalid fixture digest")
            files[algo + "-" + digest.lower()] = (algo, entry)
        print(label, version["version_number"], version["id"], manifest["dependencies"], flush=True)
    print("Unique payload files:", len(files), "(~%.1f MiB)" % (sum(f["fileSize"] for _, f in files.values()) / 2**20), flush=True)
    if not args.download_files:
        print("Pass --download-files to prepare the complete acceptance test.")
        return
    cache = args.output / "cache"
    cache.mkdir(exist_ok=True)

    def download(item):
        name, (algo, f) = item
        target = cache / name
        expected = f["hashes"][algo].lower()
        if target.exists() and hashlib.new(algo, target.read_bytes()).hexdigest() == expected:
            return
        fetch(f["downloads"][0], target)
        if hashlib.new(algo, target.read_bytes()).hexdigest() != expected:
            target.unlink()
            raise ValueError("Checksum mismatch: " + f["path"])
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(download, files.items()))
    print("All payload files verified.")


if __name__ == "__main__":
    main()
