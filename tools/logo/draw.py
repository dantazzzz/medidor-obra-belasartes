from PIL import Image, ImageDraw, ImageFont
S=360
img=Image.new("RGBA",(S,S),(0,0,0,0))
d=ImageDraw.Draw(img)
W=(255,255,255,255)
def line(p,w): d.line(p,fill=W,width=w,joint="curve")
# triangulo duplo
line([(180,26),(36,304),(324,304),(180,26)],7)
line([(180,58),(64,288),(296,288),(180,58)],3)
try:
    fb=ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf",46)
    f2=ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf",30)
except: fb=f2=ImageFont.load_default()
def ctext(y,s,f):
    w=d.textlength(s,font=f); d.text((180-w/2,y),s,font=f,fill=W)
ctext(96,"FEBASP",fb)
ctext(150,"1925",f2)
# pediment (fronton) + colunas (predio neoclassico)
d.polygon([(150,206),(210,206),(180,184)],outline=W,width=4)
d.rectangle([(150,206),(210,212)],fill=W)
for x in (156,168,180,192,204):
    d.rectangle([(x-2,214),(x+2,262)],fill=W)
d.rectangle([(146,262),(214,270)],fill=W)
# loureiros (dois ramos curvos com folhinhas)
import math
def laurel(cx,cy,dirx):
    pts=[]
    for i in range(0,9):
        t=i/8.0
        x=cx+dirx*(20+70*t); y=cy- (40*math.sin(t*math.pi))
        pts.append((x,y))
    line(pts,3)
    for i in range(1,8):
        x,y=pts[i]
        d.ellipse([x-6,y-9,x+6,y+3],outline=W,width=2)
laurel(150,300,-1)
laurel(210,300, 1)
img.save(r'C:/Users/muril_of9xvib/OneDrive/Documentos/Arduino/NivelDigital/_logo/emblem.png')
print("ok",img.size)
