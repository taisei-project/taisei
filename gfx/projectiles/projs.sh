#!/bin/sh

# **projectile generation script**
# requires imagemagick

# Ball
convert -size 27x27 xc:transparent -fill white -draw "circle 13,13 20,20" ball.png
convert -size 27x27 xc:transparent \
	\( -fill white -draw "circle 13,13 20,20" \) \
	\( radial-gradient: \) \
	-compose dst-out -composite ballc.png