#!/usr/bin/env python3

#uses images cut using https://www.imgonline.com.ua/eng/cut-photo-into-pieces.php

import sys, os
from PIL import Image

cdir = os.getcwd()
min_width = 0
min_height = 0
padding = 1
next_arg = None
for arg in sys.argv[1:]:
  if next_arg == '-b':
    make_bw = True
    bw_thresh = int(arg)
    next_arg = None
  elif next_arg == '-w':
    min_width = int(arg)  
    next_arg = None
  elif next_arg == '-h':
    min_height = int(arg)  
    next_arg = None
  elif next_arg == '-p':
    padding = int(arg)  
    next_arg = None
  elif arg[0] == '-':
    next_arg = arg
  else:
    cdir = arg
  #endif
#endfor  

files = os.listdir(cdir)
for fid in files:
  try:
    im = Image.open(fid)
  except:
    print("Failed to open ",fid)
    continue
  #endtry
  width, height = im.size
  print("Image:",fid," Size:",im.size)
  start_height = 0
  px = im.load()
  fnm = os.path.basename(fid)
  nm, ext = os.path.splitext(fnm)
  fnm = "img_" + nm
  ln = "PROGMEM unsigned char " + fnm + "[] = {\n"
  i = 0
  byte_cnt = 0
  vt = 0
  for y in range(height):
    xi = 0
    for x in range(max(min_width,width)-1,-1,-1):
      if x < width and y < height:
        #print(px[x,y])
        v = px[x,y]
        if len(v) > 1:
          v = v[0]        
        if v > bw_thresh:
          v = 1
        else:
          v = 0
        #endif
      else:
        v = padding
      #endif
      if (i & 7) == 0:
        if xi > 0:
          ln += "0x%02x,"%(vt,)
          byte_cnt += 1
        #endif  
        vt = v
      else:
        vt |= v << (i & 7)
        #print(i,vt,v,i & 7)
      #endif
      i += 1
      xi += 1
    #endfor
    print("End of run:",i,byte_cnt,i & 7)
    #assert False
    while (i & 7) != 0:  #finish trailing bytes
      vt |= padding << (i & 7)
      i += 1
    #endwhile
    print("Now:",i,i & 7)
    if (i > 0) and ((i & 7) == 0):
      print("adding byte")
      ln += "0x%02x,"%(vt,)
      byte_cnt += 1
    #endif  
    #else:
    #  width -= 1  # strange quirk    
    #endif  
    ln += "\n"
  #endfor
  
  ln = ln[:-2]  #remove comma
  ln += "};\nunsigned int %s_len=%d;\nunsigned int %s_width=%d;\nunsigned int %s_height=%d;\n"%(fnm,byte_cnt,fnm,xi,fnm,height)
  ofid = os.path.join(os.path.dirname(fid),fnm+".h")
  print("Writing to:",ofid)
  f = open(ofid,"w")
  f.write(ln)
  f.close()
#endfor  
  