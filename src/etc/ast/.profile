# Login shell profile.

umask 022

# Favourite editor and pager, search path for binaries, etc.
export EDITOR=vi
export PAGER=less

# Let cd display the current directory on the status line.
if [ -t 0 -a -f /usr/bin/tget ] && tget -flag hs
then
case $- in *i*)
	hostname=$(expr $(uname -n) : '\([^.]*\)')
	eval "cd()
	{
		chdir \"\$@\" &&
		echo -n '$(tget -str ts \
				"$USER@$hostname:'\"\`pwd\`\"'" \
				-str fs)'
	}"
	unset hostname
	cd .
	;;
esac
fi

# Check terminal type.
case $TERM in
dialup|unknown|network)
	echo -n "Terminal type? ($TERM) "; read term
	TERM="${term:-$TERM}"
	unset term
esac

# customize shell
cd() {
	command cd "$@"
	PS1="$(pwd) > "
}
PS1="$(pwd) > "

export HOME=/home/
cd

mkdir -p /mnt/shared
mount -t vbfs -o share=share none /mnt/shared

export RSYNC="rsync -cr --progress"
alias ossync="$RSYNC /mnt/shared/src/etc/ /home/repo/src/etc/ && $RSYNC /mnt/shared/src/include/ /home/repo/src/include/ && $RSYNC /mnt/shared/src/minix/ /home/repo/src/minix/ && $RSYNC /mnt/shared/src/sys/ /home/repo/src/sys/ && $RSYNC --exclude "src" --exclude ".git" /mnt/shared/ /home/repo/"
alias recomp="cd /home/repo/src/releasetools && make hdboot"
