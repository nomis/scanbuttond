#!/bin/sh

# This script is started by scanbuttond whenever a scanner button has been pressed.
# Scanbuttond passes the following parameters to us:
# $1 ... the button number
# $2 ... the scanner's SANE device name, which comes in handy if there are two or 
#        more scanners. In this case we can pass the device name to SANE programs 
#        like scanimage.

case $1 in
	1)	# button 1 has been pressed
		# print a grayscale copy
		scanimage --device-name $2 --format tiff --mode Gray --quick-format A4 \
		--resolution 300 --sharpness 0 --brightness -3 \
		--gamma-correction "High contrast printing" > /tmp/scan.tiff
		tiff2ps -2 -p -z -w 8.27 -h 11.69 /tmp/scan.tiff | lpr
		;;
	2)
		echo "button 2 has been pressed on $2"
		;;
	3)
		echo "button 3 has been pressed on $2"
		;;
	4)
		echo "button 4 has been pressed on $2"
		;;
esac

