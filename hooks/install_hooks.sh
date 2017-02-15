#!/bin/bash

# install git hooks

src_dir=../../hooks
dst_dir=../.git/hooks

if test ! \( -x $dst_dir/pre-commit -a -L $dst_dir/pre-commit \);

then

	rm -f $dst_dir/pre-commit

	if ! ln -s $src_dir/pre-commit.hook $dst_dir/pre-commit 2> /dev/null

	then

		echo "Failed to create commit hook symlink, copying instead ..."
		cp pre-commit.hook ../.git/hooks/pre-commit

	fi

fi
