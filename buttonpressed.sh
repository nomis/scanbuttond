#!/bin/sh

case $1 in
	1)	# button 1 has been pressed
		# print a grayscale copy
		scanimage --format tiff --mode Gray --quick-format A4 --resolution 300 --sharpness 0 --brightness -3 --gamma-correction "High contrast printing" > /tmp/scan.tiff
		tiff2ps -2 -p -z -w 8.27 -h 11.69 /tmp/scan.tiff | lpr
		;;
	2)
		echo "button 2 has been pressed"
		;;
	3)
		echo "button 3 has been pressed"
		;;
	4)
		echo "button 4 has been pressed"
		;;
esac

