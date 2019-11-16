import os

f = open("../src/game/client/components/duck_lua.cpp")
src_contents = f.read()
f.close()

output = """
Lua API
==============

The Lua back end is based on `LuaJit 2.0.5 <http://luajit.org/>`_.

==============
Functions
==============

|

"""

cur = src_contents
start = cur.find("/*#")
while start != -1:
    cur = cur[start+3:]
    end = cur.find("#*/")

    if end == -1:
        print("ERROR: #*/ missing")
        break

    block = cur[:end]
    proto_start = block.find("`")
    if proto_start != -1:
        block = block[proto_start+1:]
        proto_end = block.find("`")
        if proto_end == -1:
            print("ERROR: ` missing")
            break

        output += block[:block.find("(")] + "\n"
        output += "---------------------------------------------------------------------\n"
        output += ".. code-block:: js\n   \n   function " + block[:proto_end]
        output += "\n"
        block = block[proto_end+1:]

    output += block
    output += "\n|\n\n"

    cur = cur[end+3:]
    start = cur.find("/*#")

f = open("source/api.rst", "w")
f.write(output)
f.close()