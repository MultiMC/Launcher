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
    echo "$GAME_LIBRARY_PATH"
    while read line; do
        [ "$line" == "" ] && continue
	path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$GAME_LIBRARY_PATH" | tr ':' '\n')"
    echo "$GAME_PRELOAD"
    while read line; do
        [ "$line" == "" ] && continue
        line="$(dirname "$line")" # Binding the parent directory is easier than messing around with an unknown number of file descriptors
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'" # even if it *is* somewhat lazy
    done <<< "$(echo "$GAME_PRELOAD" | tr ':' '\n')"
    echo "$LD_LIBRARY_PATH"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$LD_LIBRARY_PATH" | tr ':' '\n')"
    echo "$LD_PRELOAD"
    while read line; do
        [ "$line" == "" ] && continue
	line="$(dirname "$line")"
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$LD_PRELOAD" | tr ':' '\n')"
    echo "$QT_PLUGIN_PATH"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$QT_PLUGIN_PATH" | tr ':' '\n')"
    echo "$QT_FONTPATH"
    while read line; do
        [ "$line" == "" ] && continue
        path_fwd="$path_fwd --ro-bind-try '$line' '$line'"
    done <<< "$(echo "$QT_FONTPATH" | tr ':' '\n')"

    for f in /etc/java*/; do
        path_fwd="$path_fwd --ro-bind $f $f"
    done

	# This sandbox script was written to provide as much isolation as reasonably possible for a MultiMC instance
	# However, it's not as well-restricted as I'd like. I'd prefer to replace `--ro-bind / /` with more specific binds
	# to limit MultiMC's read access to the host filesystem as much as possible.
	# Filesystem permissions should be sufficient in most cases, but I prefer not to even give that much access
	#
	# Security considerations include:
	# - This script doesn't restrict access to the host's root as much as I'd like
	# - This script doesn't perform any X11 sandboxing at all
	# - This script doesn't implement seccomp, so the kernel attack surface isn't restricted at all
	# The main motivation for this was the (not-so-)recent log4j vulnerability. I'd like a way to restrict what someone
	# can do if they manage to successfully infect my system through a Minecraft/Java vulnerability, especially if I
	# intend to play online on older Minecraft versions that will likely never see a log4j fix
    bwrap --die-with-parent --cap-drop all --new-session --unshare-all --share-net \
        --tmpfs / --tmpfs /tmp --tmpfs /home --tmpfs /root --dev-bind /dev /dev --proc /proc \
        --ro-bind /usr/bin /usr/bin --bind /bin /bin --ro-bind-try /etc/alternatives /etc/alternatives \
        --ro-bind /lib /lib --ro-bind-try /lib64 /lib64 --ro-bind-try /lib32 /lib32 \
        --ro-bind /usr/lib /usr/lib \
        $path_fwd --bind "$INSTDIR" "$INSTDIR" \
        --ro-bind-try /run/user/"$UID"/pulse /run/user/"$UID"/pulse \
        --ro-bind-try /run/user/"$UID"/pipewire-0 /run/user/"$UID"/pipewire-0 \
        --ro-bind-try /tmp/.X11-unix/X0 /tmp/.X11-unix/X0 \
        --ro-bind-try /run/user/"$UID"/wayland-1 /run/user/"$UID"/wayland-1 \
        --ro-bind /etc/fonts /etc/fonts \
        --ro-bind /usr/share/icons /usr/share/icons \
        --ro-bind /usr/share/mime /usr/share/mime \
        --ro-bind /usr/share/fontconfig /usr/share/fontconfig \
        --ro-bind /usr/share/fonts /usr/share/fonts \
        --ro-bind /etc/resolv.conf /etc/resolv.conf \
        --ro-bind /etc/ssl /etc/ssl \
        --ro-bind /etc/ca-certificates /etc/ca-certificates \
        --ro-bind /usr/share /usr/share \
    ./MultiMC "$@"
}

if [[ ! -f ${INSTDIR}/MultiMC ]]; then
    deploy
    runmmc "$@"
else
    runmmc "$@"
fi
