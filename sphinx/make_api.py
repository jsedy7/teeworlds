import os

f = open("../src/game/client/components/duck_js.cpp")
src_contents = f.read()
f.close()

output = """
Javascript API
==============

The Js back end is based on `Duktape 2.4.0 <https://duktape.org>`_.

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