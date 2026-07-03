import sys
import os
import re
import glob
from pathlib import Path

print(sys.argv, flush=True)

def split_paths(paths: str):
  out: list[str] = []
  s: int = 0
  while True:
    p = paths.find(";")
    #print(f"{s}:{p}")
    P: Path
    if p < 0:
      P = Path(paths)
    else:
      P = Path(paths[:p])

    if not P.is_file():
      print(f"ignoring {P}")
    else:
      out.append(str(P.resolve()))

    if p < 0:
      break
    paths = paths[p+1:]
    s = p + 1
  return out

headers: list[tuple[str, bool]] = []
sources: list[str] = []
defines: list[tuple[str, str]] = []
written: dict[str, str] = {}
out: str = ""
imp: str = ""
guard: str = ""
skip: bool = False
for i, v in enumerate(sys.argv):
  if v == "-o":
    out = sys.argv[i+1]
    skip = True
    continue
  elif v == "-d":
    imp = sys.argv[i+1]
    skip = True
    continue
  elif v == "-g":
    guard = sys.argv[i+1]
    skip = True
    continue
  elif v == "-D":
    D = sys.argv[i+1]
    p = D.find("=")
    if p < 0:
      defines.append((D, ""))
    else:
      defines.append((D[:p], D[p+1:]))
    skip = True
    continue
  elif v == "-h":
    for fi in split_paths(sys.argv[i+1]):
      headers.append((fi, False))
    skip = True
    continue
  elif i > 0 and not skip:
    print(v)
    for fi in split_paths(v):
      sources.append(fi)
  skip = False

if out == "":
  print("No output file given, using mini.hpp (-o <file>)")
  out = "mini.hpp"
if imp == "":
  print("No implementation definition given, using MINI_IMPL (-d <definition>)")
  imp = "MINI_IMPL"
if guard == "":
  guard = imp + "ED"
if len(sources) == 0:
  print("No sources")
  exit(1)

print("out: " + out)
print("headers: ")
for v in headers:
  print("  " + v[0])
print("sources: ")
for v in sources:
  print("  " + v)

increg = re.compile(r'^[\t ]*#[\t ]*include "([\.\w\-\/]+\.hp{,2})"', re.M)
sincreg = re.compile(r'#include <\w+\.hp{,2}>')

# mini algorithm:
# For every source that hasnt been visited:
#   Find every "" include. add to tmplist
#   Insert tmplist just before entry in sources
#   Reset loop to front of sources
#
# For every header in sources:
#   If header hasnt been written yet:
#     Write to output, commenting "" includes
#
# For every code source in sources:
#   W/ implementation guard:
#   Write to output, commenting "" includes

# STAMP OF MEDIOCRE APPROVAL
#   Respects multilevel header dependencies, and their sequential nature
#   Only potential issue depending on implementation, the search phase can go on really long
#   Every header graph is considered and must be considered in full.
#     But dealing with duplicates can be lessened by saving what headers they include, and inserting that list.
#
# IDK if this is still accurate, or adjacent to the current implementation


# mini IMPLEMENTATION

def mini_clean(files: list[str]):
  out: list[str] = []
  check: dict[str, str] = {}
  for v in files:
    if check.get(v):
      continue;
    check[v] = v
    out.append(v)
  return out

def mini_search():
  i: int = 0
  while i < len(headers):
    v = headers[i]
    if v[1]:
      i+=1
      continue
    headers[i] = (v[0], True)
    j: int = i
    skip: bool = False
    for line in open(v[0]):
      if line == "#pragma mini skip\n":
        skip = True
        continue
      m = increg.search(line)
      if m == None or skip:
        skip = False
        continue
      p = Path(v[0]).parent / m.groups()[0]
      ninc = str(p.resolve())
      #print(f"potential include: {ninc} from {v}")
      caninc: bool = True
      for k, h in enumerate(headers):
        if h[0] == ninc:
          caninc = False
          # Header is included, but is processed after this one,
        # so it must be moved to just before this header.
          if k > i:
            headers.insert(i, headers[k])
            headers.pop(k+1)
      if caninc:
        #print(f"adding {ninc} to headers")
        headers.insert(j, (ninc, False))
      j += 1
    if j > i:
      i = 0

def write_file(fout, file: Path, parent: Path | None):
  skip: bool = False
  if written.get(str(file)):
    return
  written[str(file)] = str(file)
  #print(f"Writing {file}")
  for line in open(file):
    if line == "#pragma mini skip\n" or skip:
      skip = True
      continue
    m = increg.search(line)
    if m == None:
      fout.write(line)
    else:
      #print(f"{m.start()}, {m.end()}, {line[m.start():m.end()]}")
      p = file.parent / m.groups()[0]
      inc = str(p.resolve())
      fout.write(f"/* {line[m.start():m.end()]} */ {line[m.end():len(line)-1]}\n\n")
      if written.get(inc) == None:
        write_file(fout, Path(inc), file)
  if parent != None:
    fout.write(f"/* {parent.name} */\n\n")
  else:
    fout.write("\n\n")

def header_write(fout):
  for v in headers:
    if written.get(v[0]):
      continue
    written[v[0]] = v[0]
    skip: bool = False
    #print(f"Writing header {v[0]}...")
    for line in open(v[0]):
      if line == "#pragma mini skip\n" or skip:
        skip = True
        continue
      skip = False
      if increg.search(line) == None:
        fout.write(line)
      else:
        fout.write(f"/*{line[:len(line)-1]}*/\n")
    fout.write("\n\n")

def source_write(fout):
  fout.write(f"#ifdef {imp}\n")
  fout.write(f"#ifndef {guard}\n#define {guard}\n\n")
  for v in sources:
    write_file(fout, Path(v), None)
  fout.write(f"#endif // #ifndef {guard}\n")
  fout.write(f"#endif // #ifdef {imp}\n")

mini_search()

fout = open(out, "w")
fout.write(f"/*  {Path(out).name} auto generated by mini.py  */\n\n/* clang-format off */\n\n")
fout.write("#ifndef MINI_GENERATED\n#define MINI_GENERATED\n#endif\n")
for v in defines:
  fout.write(f"#define {v[0]} {v[1]}\n")

fout.write("\n")

header_write(fout)
source_write(fout)


fout.write("#undef MINI_GENERATED")
fout.write("\n/* clang-format on */\n")

for file in written:
  print(f"Wrote {file}")
