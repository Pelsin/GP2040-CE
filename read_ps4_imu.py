#!/usr/bin/env python3
"""GP2040-CE MPU-6050 live IMU visualizer — rotating 3D wireframe cube"""

import ctypes, struct, sys, time, math, tty, termios, select

# ── hidapi ────────────────────────────────────────────────────────────────────
lib_path = '/opt/homebrew/lib/libhidapi.dylib'
try:
    hid = ctypes.cdll.LoadLibrary(lib_path)
except OSError:
    sys.exit(f"Cannot load {lib_path} — run: brew install hidapi")

hid.hid_open.restype  = ctypes.c_void_p
hid.hid_open.argtypes = [ctypes.c_ushort, ctypes.c_ushort, ctypes.c_void_p]
hid.hid_read_timeout.restype  = ctypes.c_int
hid.hid_read_timeout.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t, ctypes.c_int]
hid.hid_close.restype  = None
hid.hid_close.argtypes = [ctypes.c_void_p]
hid.hid_exit.restype   = None
hid.hid_init()

CANDIDATES = [
    (0x054C, 0x09CC, "DS4 v2"),
    (0x054C, 0x05C4, "DS4 v1"),
    (0x1532, 0x0401, "Razer Panthera"),
    (0x054C, 0x0CE6, "DualSense"),
]

dev = dev_label = None
for vid, pid, label in CANDIDATES:
    h = hid.hid_open(vid, pid, None)
    if h:
        dev = h
        dev_label = f"{label}  VID=0x{vid:04X}  PID=0x{pid:04X}"
        break

if not dev:
    sys.exit("Device not found. Is the Pico W plugged in and in PS4 mode?")

# ── sensor constants ──────────────────────────────────────────────────────────
GYRO_SENS  = 131.0    # LSB/(°/s) @ ±250°/s
ACCEL_SENS = 16384.0  # LSB/g     @ ±2g

# ── ANSI helpers ──────────────────────────────────────────────────────────────
def at(r, c): return f"\033[{r};{c}H"
HIDE = "\033[?25l";  SHOW = "\033[?25h";  CLR = "\033[2J\033[H"
RST  = "\033[0m";    B    = "\033[1m";    DIM = "\033[2m"
CYN  = "\033[96m";   GRN  = "\033[92m";  YLW = "\033[93m"
MAG  = "\033[95m";   BLU  = "\033[94m";  WHT = "\033[97m"

# ── 3D wireframe cube ─────────────────────────────────────────────────────────
CW, CH = 45, 21          # canvas cols / rows
CCX, CCY = CW//2, CH//2
SX, SY   = 10.0, 5.0     # scale (chars are ~2× taller than wide)

VERTS = [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
         (-1,-1, 1),(1,-1, 1),(1,1, 1),(-1,1, 1)]
EDGES = [(0,1),(1,2),(2,3),(3,0),   # front
         (4,5),(5,6),(6,7),(7,4),   # back
         (0,4),(1,5),(2,6),(3,7)]   # sides

def rotate(x, y, z, yaw, pitch, roll):
    cy,sy = math.cos(yaw),   math.sin(yaw)
    cp,sp = math.cos(pitch), math.sin(pitch)
    cr,sr = math.cos(roll),  math.sin(roll)
    x1 = x*cy - y*sy;   y1 = x*sy + y*cy;   z1 = z
    y2 = y1*cp - z1*sp; z2 = y1*sp + z1*cp; x2 = x1
    x3 = x2*cr + z2*sr; z3 =-x2*sr + z2*cr; y3 = y2
    return x3, y3, z3

def project(x, y, z):
    f = 4.5 / (4.5 + z + 2.5)
    return CCX + x*SX*f, CCY + y*SY*f

def bresenham(canvas, x0, y0, x1, y1, ch):
    x0,y0,x1,y1 = round(x0),round(y0),round(x1),round(y1)
    dx,dy = abs(x1-x0), abs(y1-y0)
    sx = 1 if x0<x1 else -1
    sy = 1 if y0<y1 else -1
    e = dx - dy
    while True:
        if 0<=x0<CW and 0<=y0<CH:
            canvas[y0][x0] = ch
        if x0==x1 and y0==y1: break
        e2 = 2*e
        if e2 > -dy: e -= dy; x0 += sx
        if e2 <  dx: e += dx; y0 += sy

def edge_ch(x0, y0, x1, y1):
    dx, dy = abs(x1-x0), abs(y1-y0)
    if dx > dy*2.0:              return '─'
    if dy > dx*0.8:              return '│'
    return '╱' if (x1-x0)*(y1-y0) < 0 else '╲'

def render_cube(yaw_deg, pitch_deg, roll_deg):
    canvas = [[' ']*CW for _ in range(CH)]
    yr = math.radians(yaw_deg)
    pr = math.radians(pitch_deg)
    rr = math.radians(roll_deg)
    pts = [project(*rotate(*v, yr, pr, rr)) for v in VERTS]
    for i, j in EDGES:
        x0,y0 = pts[i]; x1,y1 = pts[j]
        bresenham(canvas, x0,y0, x1,y1, edge_ch(x0,y0,x1,y1))
    for px,py in pts:
        ix,iy = round(px), round(py)
        if 0<=ix<CW and 0<=iy<CH:
            canvas[iy][ix] = '●'
    return [''.join(row) for row in canvas]

# ── bar chart helper ──────────────────────────────────────────────────────────
BW = 14
def bar(val, maxv):
    filled = min(BW, int(abs(val)/maxv*BW))
    sign = '+' if val >= 0 else '-'
    return f"{sign}[{'█'*filled}{'░'*(BW-filled)}]"

# ── non-blocking raw keyboard ─────────────────────────────────────────────────
fd = sys.stdin.fileno()
old_tty = termios.tcgetattr(fd)

def read_key():
    return sys.stdin.read(1) if select.select([sys.stdin],[],[],0)[0] else None

# ── layout constants ──────────────────────────────────────────────────────────
TITLE_R = 1
SEP_R   = 2
CUBE_R  = 3                  # rows 3 … 3+CH-1
STAT_R  = CUBE_R + CH + 1    # stats start here

# ── main loop ─────────────────────────────────────────────────────────────────
yaw = pitch = roll = 0.0
last_t = time.monotonic()
buf = ctypes.create_string_buffer(64)
gx = gy = gz = ax = ay = az = batt = 0

sys.stdout.write(HIDE + CLR)
sys.stdout.flush()

try:
    tty.setraw(fd)

    while True:
        k = read_key()
        if k in ('q', 'Q', '\x03', '\x1b'):
            break
        if k in ('r', 'R'):
            yaw = pitch = roll = 0.0

        n = hid.hid_read_timeout(dev, buf, 64, 16)
        now = time.monotonic()
        dt  = min(now - last_t, 0.05)
        last_t = now

        if n > 0:
            d = buf.raw[:n]
            if d[0] == 0x01 and n >= 27:
                batt,gx,gy,gz,ax,ay,az = struct.unpack_from('<H3h3h', d, 13)
                yaw   += (gz / GYRO_SENS) * dt   # Z → yaw
                pitch += (gx / GYRO_SENS) * dt   # X → pitch
                roll  += (gy / GYRO_SENS) * dt   # Y → roll

        rows = render_cube(yaw, pitch, roll)
        gxd,gyd,gzd = gx/GYRO_SENS, gy/GYRO_SENS, gz/GYRO_SENS
        axg,ayg,azg = ax/ACCEL_SENS, ay/ACCEL_SENS, az/ACCEL_SENS

        out = []

        # ── title ────────────────────────────────────────────────────────
        out.append(at(TITLE_R,1) + B+WHT +
            f"  GP2040-CE MPU-6050 Visualizer  │  {dev_label}" + RST)
        out.append(at(SEP_R,1) + DIM + "─"*72 + RST)

        # ── cube ─────────────────────────────────────────────────────────
        for i, row in enumerate(rows):
            out.append(at(CUBE_R+i, 3) + CYN + row + RST)

        # ── stats ─────────────────────────────────────────────────────────
        R = STAT_R
        out += [
            at(R,  1)+DIM +"  ─── GYROSCOPE (°/s) " + "─"*47+RST,
            at(R+1,1)+GRN +f"  Pitch X  {gxd:+8.2f}  {bar(gxd,500)}"+RST,
            at(R+2,1)+GRN +f"  Roll  Y  {gyd:+8.2f}  {bar(gyd,500)}"+RST,
            at(R+3,1)+GRN +f"  Yaw   Z  {gzd:+8.2f}  {bar(gzd,500)}"+RST,
            at(R+4,1)+DIM +"  ─── ACCELEROMETER (g) " + "─"*45+RST,
            at(R+5,1)+MAG +f"  X {axg:+7.4f}g   Y {ayg:+7.4f}g   Z {azg:+7.4f}g"+RST,
            at(R+6,1)+DIM +"  ─── INTEGRATED ANGLE " + "─"*46+RST,
            at(R+7,1)+BLU +f"  Yaw {yaw%360:6.1f}°   Pitch {pitch%360:6.1f}°   Roll {roll%360:6.1f}°"+RST,
            at(R+9,1)+DIM +"  [R] reset angles    [Q / Ctrl+C] quit"+RST,
        ]

        sys.stdout.write(''.join(out))
        sys.stdout.flush()
        time.sleep(0.016)

finally:
    termios.tcsetattr(fd, termios.TCSADRAIN, old_tty)
    sys.stdout.write(SHOW + RST + CLR)
    hid.hid_close(dev)
    hid.hid_exit()
