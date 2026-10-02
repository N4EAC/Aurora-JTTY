from pathlib import Path
from html import escape
root=Path(__file__).resolve().parent
items=[]
def box(x,y,w,h,fill='#e9ebee',stroke='#adb2b9',radius=7):items.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}"/>')
def txt(x,y,s,size=15,color='#29323a',weight=400):items.append(f'<text x="{x}" y="{y}" font-family="Arial, Helvetica, sans-serif" font-size="{size}" font-weight="{weight}" fill="{color}">{escape(s)}</text>')
def button(x,y,w,s,fill='#f1f2f4',color='#29323a'):box(x,y,w,32,fill);txt(x+10,y+21,s,14,color)
box(0,0,1280,930,'#cdd1d6','#cdd1d6',12)
for x,c in [(23,'#e56b68'),(44,'#d7ae57'),(65,'#68ae80')]:items.append(f'<circle cx="{x}" cy="22" r="6" fill="{c}"/>')
txt(94,28,'JTTY Workbench',18,weight=700);txt(974,27,'LAYOUT PROPOSAL · SAMPLE DATA',12,'#596570')
box(16,47,1248,112,'#bfc5cc')
txt(32,72,'RADIO FREQUENCY',11,'#52606d',700);txt(32,112,'7.090000',32,weight=700);txt(203,112,'MHz',16)
button(32,119,219,'40 m · JTTY 7.090 MHz  ▾')
txt(282,72,'RECEIVE OFFSET',11,'#52606d',700);button(282,83,132,'1500 Hz   ↕')
txt(434,72,'TRANSMIT OFFSET',11,'#52606d',700);button(434,83,132,'1500 Hz   ↕')
txt(585,72,'RX TOLERANCE',11,'#52606d',700);button(585,83,132,'± 100 Hz   ↕')
button(282,122,132,'Copy Rx → Tx');button(434,122,132,'Copy Tx → Rx')
txt(585,143,'Mode: DATA USB',13)
button(759,81,140,'● CAT connected','#dbe6df','#315d43');button(759,122,140,'Radio / Audio…')
button(918,81,144,'Stop Monitor','#dde7e0','#315d43');button(1075,81,168,'Halt Tx','#eee0dd','#874339')
button(918,122,144,'Tune');button(1075,122,168,'Log contact')
# Two message panels under the frequency strip.
box(16,175,410,482,'#e7e9ed');box(438,175,826,482,'#edf0f2')
txt(33,205,'Received messages',19,weight=700);txt(33,230,'All decoded signals in the waterfall range',12,'#62707d');button(335,187,74,'Clear')
box(29,245,384,29,'#cbd1d8',radius=3);txt(41,265,'UTC',12,weight=700);txt(99,265,'Hz',12,weight=700);txt(162,265,'Message',12,weight=700)
rows=[('10:12','1125','CQ K2ABC K2ABC'),('10:13','1500','CQ N4EAC N4EAC'),('10:13','1870','DE W1XYZ GOOD MORNING'),('10:14','1502','N4EAC DE K2ABC HELLO'),('10:14','1498','YOUR SIGNAL IS CLEAR HERE')]
for i,(t,f,m) in enumerate(rows):
 y=300+i*54
 if f in ['1500','1502','1498']:box(33,y-23,376,43,'#dce3e9','#dce3e9',4)
 txt(41,y,t,12,'#5b6772');txt(99,y,f,13);txt(162,y,m,12)
txt(34,615,'Select a signal using the waterfall.',13,'#62707d');txt(34,638,'Highlighted rows are near your Rx offset.',12,'#62707d')
txt(456,205,'Conversation',20,weight=700);txt(456,230,'Rx 1500 Hz · ±100 Hz · traffic near this frequency',13,'#62707d');button(1044,187,101,'Pop out ↗');button(1154,187,91,'Clear')
# Chat transcript, preserving chronological frequency scope rather than inventing callsign isolation.
box(455,244,791,227,'#f4f5f6','#bdc4cb',5)
entries=[('10:13:20','TX · 1500 Hz','CQ N4EAC N4EAC','#8a602c'),('10:14:02','RX · 1502 Hz','N4EAC DE K2ABC HELLO','#3d627d'),('10:14:19','TX · 1500 Hz','K2ABC DE N4EAC GOOD MORNING','#8a602c'),('10:14:43','RX · 1498 Hz','YOUR SIGNAL IS CLEAR HERE','#3d627d')]
for i,(time,tag,msg,col) in enumerate(entries):
 y=274+i*51;txt(470,y,time,12,'#697681');txt(550,y,tag,12,col,700);txt(550,y+22,msg,16)
txt(456,495,'Message',12,'#52606d',700);txt(1148,495,'0 / 80 characters',11,'#697681')
box(455,505,625,41,'#ffffff','#8695a4',5);txt(469,531,'Type a message… Enter to send',15,'#7b8691');button(1091,509,155,'Send message','#dbe4ed','#324d65')
for i in range(8):
 x=455+i*99;button(x,557,91,f'F{i+1}');txt(x+5,607,['%M CQ','%H %E','TU CQ','%M','%H','%Q %E','AGAIN?','%E'][i],12,'#556471')
txt(456,639,'Their call: K2ABC      Call next: —      Serial: 1',13);txt(1010,639,'Lower case  ☐   Include time  ☑',12)
# Existing waterfall moved into the same main window, not a second capture engine.
box(16,673,1248,212,'#bfc5cc');txt(32,700,'Waterfall',17,weight=700);txt(155,700,'Click a signal to set Rx offset',13,'#596570');button(967,681,130,'Display controls');txt(1115,701,'Input −28 dBFS',13)
box(31,716,1218,151,'#18232f','#89949e',3)
for i in range(11):txt(43+i*111,739,str(200+i*250),11,'#b2c0cc')
for i in range(28):
 y=745+i*4;items.append(f'<path d="M32 {y} H1248" stroke="#27384a" stroke-width="1"/>')
for x in [443,624,790]:
 for off in [-4,-2,0,2,4]:items.append(f'<path d="M{x+off} 746 V866" stroke="'+('#a0bdc9' if off==0 else '#436b80')+'" stroke-width="1"/>')
items.append('<path d="M624 716 V867" stroke="#e5bf70" stroke-width="2"/>')
txt(33,911,'RX · USB Audio Device · Mono     |     OUT · USB Audio Device     |     CAT · FT-710',13,'#455462');txt(1002,911,'No transmission in progress',13,'#455462')
svg='<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="930" viewBox="0 0 1280 930">'+''.join(items)+'</svg>'
(root/'jtty-layout-proposal.svg').write_text(svg)
