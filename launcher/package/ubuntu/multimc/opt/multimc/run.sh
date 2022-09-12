#!/bin/bash

INSTDIR="${XDG_DATA_HOME-$HOME/.local/share}/multimc"

if [ `getconf LONG_BIT` = "64" ]
then
    PACKAGE="mmc-stable-lin64.tar.gz"
else
    PACKAGE="mmc-stable-lin32.tar.gz"
fi

deploy() {
    mkdir -p $INSTDIR
    cd ${INSTDIR}

    wget --progress=dot:force "https://files.multimc.org/downloads/${PACKAGE}" 2>&1 | sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' | zenity --progress --auto-close --auto-kill --title="Downloading MultiMC..."

    tar -xzf ${PACKAGE} --transform='s,MultiMC/,,'
    rm ${PACKAGE}
    chmod +x MultiMC
}

runmmc() {
    cd ${INSTDIR}
    source ./MultiMC --dry-run
    path_fwd=""
    # Now to setup binds for the execution environment
    # This allows the sandbox to dynamically grant permission to PATH entries and preloaded libraries
    # This would be more useful if we weren't already `ro-bind`ing the entire filesystem root, but
    # at least the boilerplate is completed for when someone eventually restricts permissions
    # more granularly
    while read line; do
        [ "$line" == "" ] && continue
	path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$GAME_LIBRARY_PATH" | tr ':' '\n')"
    while read line; do
        [ "$line" == "" ] && continue
        line="$(dirname "$line")" # Binding the parent directory is easier than messing around with an unknown number of file descriptors
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'" # even if it *is* somewhat lazy
    done <<< "$(echo "$GAME_PRELOAD" | tr ':' '\n')"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$LD_LIBRARY_PATH" | tr ':' '\n')"
    while read line; do
        [ "$line" == "" ] && continue
	line="$(dirname "$line")"
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$LD_PRELOAD" | tr ':' '\n')"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$QT_PLUGIN__PATH" | tr ':' '\n')"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$QT_FONTPATH" | tr ':' '\n')"
        # This sandbox script was written to provide as much isolation as reasonably possible for a MultiMC instance
        # However, it's not as well-restricted as I'd like. I'd prefer to replace `--ro-bind / /` with more specific binds
        # to limit MultiMC's read access to the host filesystem as much as possible.
        # Filesystem permissions should be sufficient in most cases, but I prefer not to even give that much access
        # since this could still allow an attacker to see system configuration information or read system files that the user
        # has read access to, such as potentially-sensitive configuration files
        #
        # Security considerations include:
        # - This script doesn't restrict access to the host's root as much as I'd like
        # - This script doesn't perform any X11 sandboxing at all
        #   - This means attacks like X11 keylogging are still a risk, even from within the sandbox. So don't type your root password while MultiMC is open
        #   - Systems running Wayland are implicitly immune to X11 keylogging, except for Sway, which is actually still vulnerable to X11 keylogging
        # - This script doesn't implement seccomp, so the kernel attack surface isn't restricted at all
        #   - This shouldn't be too big of a deal if you trust your kernel to be secure enough. The threat model is mostly skiddies and low-effort attacks so I think it's fine-ish
        #
        # The main motivation for this was the (not-so-)recent log4j vulnerability. I'd like a way to restrict what someone
        # can do if they manage to successfully infect my system through a Minecraft/Java vulnerability, especially if I
        # intend to play online on older Minecraft versions that will likely never see a log4j fix
        # While I'm aware that MultiMC already has a patched version of log4j, I think it makes sense to additionally
        # sandbox the entire launcher
        #
        # This has the additional benefit of mitigating potential future exploits that we don't know about yet
        # Note that this adds an additional dependency to the launcher: Bubblewrap. This is unavoidable if we want to use Bubblewrap to sandbox the game
    bwrap --die-with-parent --cap-drop all --new-session --unshare-all --share-net \
        --ro-bind / / --tmpfs /tmp --tmpfs /home --tmpfs /root --dev-bind-try /dev /dev --proc /proc \
        $path_fwd --bind "$INSTDIR" "$INSTDIR" \
    ./MultiMC "$@"
}

if [[ ! -f ${INSTDIR}/MultiMC ]]; then
    deploy
    runmmc "$@"
else
    runmmc "$@"
fi
