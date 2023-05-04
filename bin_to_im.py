import png2bin
from PIL import Image

# k = 50
# for i in range(k):
#     filename = "img" + str(i)
#     bin = filename + ".bin"
#     image = png2bin.read_image(bin)
#     png = filename + ".png"
#     image.save(png)

filename = "img"
bin = filename + ".bin"
image = png2bin.read_image(bin)
png = filename + ".png"
image.save(png)

