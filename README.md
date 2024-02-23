# Make Windows Icon

Generates a windows icon file (.ico) given a single image file. It generates  
the recommended sizes of 16, 32, 48, and 256 pixel square images. It defaults  
to embedding images in bmp format, but can also embed images in png format by  
passing the --png flag after the input filename. Embedding images in png format  
will result in a smaller file size.  

Input images can be in any format handled by stb_image for example: png, gif,  
bmp, and most jpeg among others.  

