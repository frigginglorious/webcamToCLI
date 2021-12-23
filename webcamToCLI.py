#!/usr/bin/env python3

import cv2
import os
import aalib
from PIL import Image
import sys

cam = cv2.VideoCapture(0)

size = os.get_terminal_size()
# screen = aalib.AnsiScreen(width=size[0], height=size[1])
# screen = aalib.AsciiScreen(width=size[0], height=size[1])
screen = aalib.LinuxScreen(width=size[0], height=size[1])


minOutLen = sys.maxsize



while True:

    check, frame = cam.read()

    myimage = Image.fromarray(frame).convert('L').resize(screen.virtual_size)

    screen.put_image((0, 0), myimage)

    textOut = screen.render()

    # Try to clear graphic stuttering
    # outLength = len(textOut)
    # minOutLen = outLength if outLength < minOutLen else minOutLen
    # textOut = textOut[:minOutLen]


    sys.stdout.write(textOut)


    key = cv2.waitKey(1)
    if key == 27:
        break



cam.release()
cv2.destroyAllWindows()
