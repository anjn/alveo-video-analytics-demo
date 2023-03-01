in=$1 ; shift
out=$(basename $in).ts
dir=$(dirname $(readlink -f $0))
set -x

opts=
opts+=" -ss 30"
opts+=" -t 60" # limit to 2min duration

maxrate=4.0M
bufsize=8.0M

ffmpeg \
	$* \
	$opts \
	-i $in \
	-c:v libx264 -preset medium -tune zerolatency -profile:v baseline -level 4.1 \
	-vf format=yuv420p,scale=1920:1080,fps=15 \
	-g 30 -bf 0 -refs 1 \
	-crf 27 \
	-bufsize $bufsize -maxrate $maxrate \
	-f mpegts \
	-an \
	$dir/$out
