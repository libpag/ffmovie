#!/bin/sh

helpFunction() {
   echo ""
   echo "Usage: $0 -t target -p platform"
   echo "-t filemovie/all"
   echo "-p mac/ios/android/all"
   echo ""
   exit 1
}

while getopts "t:p:" opt
do
	case "$opt" in
		t ) parameterT="$OPTARG" ;;
		p ) parameterP="$OPTARG" ;;
		* ) helpFunction ;;
	esac
done

option=""
if [[ ! -z $parameterT ]]; then
	option="$option -t $parameterT"
fi

case $parameterP in
	mac )
		./build_ffmpeg_mac.sh $option
		;;
	ios )
		./build_ffmpeg_ios.sh $option
		;;
	android )
		./build_ffmpeg_android.sh $option
		;;
	all )
		./build_ffmpeg_mac.sh $option
		./build_ffmpeg_ios.sh $option
		./build_ffmpeg_android.sh $option
		;;
	* )
		helpFunction
		;;
esac
