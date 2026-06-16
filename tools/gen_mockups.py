# Gera mockups (SVG) das telas do Medidor de Obra para a documentacao.
import os
OUT = os.path.join(os.path.dirname(__file__), "..", "docs", "img")
os.makedirs(OUT, exist_ok=True)

def save(name, content):
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 412 412" '
           'font-family="Segoe UI,Arial,sans-serif">'
           '<circle cx="206" cy="206" r="205" fill="#05070a" stroke="#1f2937" stroke-width="3"/>'
           + content + '</svg>')
    with open(os.path.join(OUT, name), "w", encoding="utf-8") as f:
        f.write(svg)

def T(x, y, s, fill="#e5e7eb", size=18, w="400", anchor="middle"):
    return (f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" '
            f'font-weight="{w}" text-anchor="{anchor}">{s}</text>')

BAR = T(206, 44, "12:34    85%", "#5b6673", 15)

def BTNS(labels=("MENU", "ZERAR", "HOLD", "SALVAR")):
    xs = [101, 171, 241, 311]
    cols = ["#374151", "#2563eb", "#1f2937", "#16a34a"]
    s = ""
    for cx, l, c in zip(xs, labels, cols):
        s += f'<rect x="{cx-31}" y="300" width="62" height="42" rx="21" fill="{c}"/>'
        s += T(cx, 326, l, "#fff", 12)
    return s

RING = ('<circle cx="206" cy="206" r="118" fill="none" stroke="#3a4452" stroke-width="2"/>'
        '<line x1="96" y1="206" x2="316" y2="206" stroke="#2a313a" stroke-width="2"/>'
        '<line x1="206" y1="100" x2="206" y2="300" stroke="#2a313a" stroke-width="2"/>'
        '<circle cx="206" cy="206" r="32" fill="none" stroke="#2f6b43" stroke-width="2"/>')

def BUB(dx=0, dy=0, c="#22c55e"):
    return f'<circle cx="{206+dx}" cy="{206+dy}" r="22" fill="{c}" stroke="#0b3d22" stroke-width="2"/>'

def tool(name, big, unit, dx=0, dy=0, bubc="#22c55e", bigc="#ffffff",
         status="", statusc="#22c55e", hint="", norm="", ring=True, bub=True):
    c = BAR + T(206, 82, name, "#9aa7b4", 22, "700")
    c += T(206, 134, big, bigc, 46, "700") + T(206, 168, unit, "#9aa7b4", 18)
    if norm: c += T(206, 190, norm, "#38bdf8", 13)
    if ring: c += RING
    if bub:  c += BUB(dx, dy, bubc)
    if status: c += T(206, 262, status, statusc, 18)
    if hint:   c += T(206, 286, hint, "#5b6673", 13)
    c += BTNS()
    return c

save("nivel.svg",        tool("NIVEL", "1.2", "GRAUS", dx=-26, dy=14))
save("prumo.svg",        tool("PRUMO", "0.4", "GRAUS", dx=0, dy=-18))
save("declividade.svg",  tool("DECLIVIDADE", "2.0", "%", dx=0, dy=-22,
                              bigc="#22c55e", norm="RAMPA NBR9050  ate 8.3%",
                              status="DENTRO DA NORMA"))
save("transferidor.svg", tool("TRANSFERIDOR", "45.0", "GRAUS", dx=30, dy=-20))
save("ruido.svg",        tool("RUIDO", "62", "dB", ring=False, bub=False,
                              bigc="#f59e0b", status="ATENCAO", statusc="#f59e0b",
                              hint="min 48   max 70"))
save("conversor.svg",    tool("CONVERSOR", "5.0", "GRAUS", dx=-20, dy=10,
                              status="8.7%    87 mm/m", statusc="#9aa7b4",
                              hint="proporcao  1:11"))
save("esquadro.svg",     tool("ESQUADRO", "90.0", "GRAUS", dx=28, dy=-22,
                              bigc="#22c55e", bubc="#4ade80",
                              status="ESQUADRO OK (90)"))
save("planeza.svg",      tool("PLANEZA", "0.8", "GRAUS", dx=-18, dy=12,
                              status="desvio: 1.3 graus", statusc="#38bdf8",
                              hint="min 0.2   max 1.5"))
save("perfil.svg",       tool("PERFIL", "2.1", "%", dx=0, dy=-20,
                              status="media ~1.8%", statusc="#0891b2",
                              hint="min 0.5%   max 3.1%"))

# --- MENU ---
def menu():
    c = T(206, 66, "Medidor de Obra", "#f1f5f9", 20, "700")
    c += T(206, 92, "Belas Artes", "#475569", 14)
    c += '<rect x="30" y="140" width="40" height="150" rx="16" fill="#0e131c" stroke="#1e293b" stroke-width="1"/>'
    c += '<rect x="342" y="140" width="40" height="150" rx="16" fill="#0e131c" stroke="#1e293b" stroke-width="1"/>'
    c += '<rect x="86" y="120" width="240" height="190" rx="24" fill="#0e131c" stroke="#1e293b" stroke-width="1"/>'
    c += '<rect x="183" y="160" width="46" height="5" rx="2" fill="#0891b2"/>'
    c += T(206, 215, "DECLIVIDADE", "#f1f5f9", 22, "700")
    c += T(206, 243, "caimento %", "#64748b", 14)
    x = 150
    for i in range(8):
        w = 16 if i == 2 else 7
        col = "#e2e8f0" if i == 2 else "#374151"
        c += f'<rect x="{x}" y="338" width="{w}" height="7" rx="3" fill="{col}"/>'
        x += w + 6
    return c
save("menu.svg", menu())

# --- SOL (carta solar polar) ---
def sol():
    cx, cy, R = 206, 178, 92
    c = T(206, 58, "SOL / INSOLACAO", "#f59e0b", 18, "700")
    c += f'<circle cx="{cx}" cy="{cy}" r="{R}" fill="none" stroke="#334155" stroke-width="2"/>'
    c += f'<circle cx="{cx}" cy="{cy}" r="{R//2}" fill="none" stroke="#1f2937" stroke-width="2"/>'
    c += T(cx, cy-R-6, "N", "#64748b", 13) + T(cx+R+10, cy+4, "L", "#64748b", 13)
    c += T(cx, cy+R+16, "S", "#64748b", 13) + T(cx-R-10, cy+4, "O", "#64748b", 13)
    c += f'<path d="M {cx-R+8} {cy+28} Q {cx} {cy-66} {cx+R-8} {cy+28}" fill="none" stroke="#38bdf8" stroke-width="3"/>'
    c += f'<line x1="{cx}" y1="{cy}" x2="{cx}" y2="{cy-R}" stroke="#f59e0b" stroke-width="2" stroke-dasharray="4 4"/>'
    c += f'<circle cx="{cx+28}" cy="{cy-34}" r="9" fill="#facc15" stroke="#713f12" stroke-width="2"/>'
    c += T(206, 302, "nascer 06:12   por 17:48", "#e5e7eb", 14)
    c += T(206, 324, "fachada 4.2h   sombra 1.3m", "#9aa7b4", 13)
    return c
save("sol.svg", sol())

# --- CROQUI (mockup da pagina web num "celular") ---
def croqui():
    s = '<rect x="56" y="8" width="300" height="396" rx="28" fill="#0b0f17" stroke="#1f2937" stroke-width="3"/>'
    s += T(206, 38, "Croqui / Anotacoes", "#e5e7eb", 15, "700")
    s += '<rect x="74" y="54" width="264" height="24" rx="6" fill="#111827"/>'
    s += T(206, 71, "Parede  Luz  Tomada  Porta", "#94a3b8", 10)
    s += '<rect x="74" y="88" width="264" height="196" rx="10" fill="#0e131c" stroke="#1f2937"/>'
    s += '<rect x="120" y="124" width="172" height="120" fill="none" stroke="#e5e7eb" stroke-width="2.5"/>'
    s += T(206, 118, "4.20 m", "#38bdf8", 10) + T(206, 260, "4.20 m", "#38bdf8", 10)
    s += '<circle cx="206" cy="184" r="8" fill="#facc15"/>' + T(206, 188, "L", "#0b0f17", 9, "700")
    s += '<circle cx="135" cy="234" r="8" fill="#38bdf8"/>' + T(135, 238, "T", "#0b0f17", 9, "700")
    s += '<circle cx="277" cy="234" r="8" fill="#38bdf8"/>' + T(277, 238, "T", "#0b0f17", 9, "700")
    s += T(206, 306, "Paredes 4 (perimetro 14.40 m)", "#cbd5e1", 10)
    s += T(206, 320, "Luz 1  .  Tomada 2", "#cbd5e1", 10)
    return s
save("croqui.svg", croqui())

print("ok - mockups em docs/img/:", sorted(os.listdir(OUT)))
