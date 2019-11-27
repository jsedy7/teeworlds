import sys
from datatypes import *
import content
import network

def EmitMembers(obj):
    has_array = False
    for v in obj.variables:
        if isinstance(v, NetIntRange) or isinstance(v, NetIntAny) or isinstance(v, NetEnum) or isinstance(v, NetFlag) or isinstance(v, NetTick):
            print('\t\tLuaSetPropInteger(L(), -1, "%s", Obj.%s);' % (v.name, v.name))
        elif isinstance(v, NetString) or isinstance(v, NetStringStrict):
            print('\t\tLuaSetPropString(L(), -1, "%s", Obj.%s);' % (v.name, v.name))
        elif isinstance(v, NetBool):
            print('\t\tLuaSetPropBool(L(), -1, "%s", Obj.%s);' % (v.name, v.name))
        elif isinstance(v, NetArray):
            if not has_array:
                print('\t\tlua_newtable(L());')
                has_array = True
            else:
                print('\t\tlua_newtable(L());')

            print('\t\tfor(int i = 0; i < %d; i++)' % v.size)
            print('\t\t{')

            if isinstance(v.var, NetIntRange) or isinstance(v.var, NetIntAny) or isinstance(v.var, NetEnum) or isinstance(v.var, NetFlag) or isinstance(v.var, NetTick):
                print('\t\t\tlua_pushinteger(L(), Obj.%s[i]);' % (v.base_name))
            elif isinstance(v.var, NetString) or isinstance(v.var, NetStringStrict):
                print('\t\t\tlua_pushstring(L(), Obj.%s[i]);' % (v.base_name))
            elif isinstance(v.var, NetBool):
                print('\t\t\tlua_pushboolean(L(), Obj.%s[i]);' % (v.base_name))

            print('\t\t\tlua_rawseti(L(), -2, i + 1);')
            print('\t\t}')
            print('\t\tlua_pushstring(L(), "%s");' % v.base_name)
            print('\t\tlua_rawset(L(), -3);')
            print('\t\t')


print("// generated")
print("")
print("bool CDuckLua::MakeVanillaLuaNetMessage(int MsgID, const void* pRawMsg)\n{")

for obj in network.Messages:
    print("\tif(MsgID == %s)" % obj.enum_name)
    print("\t{")
    print("\t\tconst %s& Obj = *(%s*)pRawMsg;" % (obj.struct_name, obj.struct_name))
    print("\t\tlua_newtable(L());")
    print('\t\tLuaSetPropInteger(L(), -1, "tw_msg_id", %s);' % (obj.enum_name))
    print('\t\tLuaSetPropString(L(), -1, "tw_name", "%s");' % (obj.struct_name))

    # inheritence
    cur = obj
    while not cur.base == "":
        for b in network.Messages:
            if b.name == obj.base:
                EmitMembers(b)
                cur = b
                break
    
    EmitMembers(obj)
    
    print("\t\treturn true;")
    print("\t}")

print("\treturn false;")
print("}\n")

print("")
print("bool CDuckLua::MakeVanillaLuaNetObj(int MsgID, const void* pRawMsg)\n{")

for obj in network.Objects:
    print("\tif(MsgID == %s)" % obj.enum_name)
    print("\t{")
    print("\t\tconst %s& Obj = *(%s*)pRawMsg;" % (obj.struct_name, obj.struct_name))
    print("\t\tlua_newtable(L());")
    print('\t\tLuaSetPropInteger(L(), -1, "tw_netobj_id", %s);' % (obj.enum_name))
    print('\t\tLuaSetPropString(L(), -1, "tw_name", "%s");' % (obj.struct_name))

    # inheritence
    cur = obj
    while not cur.base == "":
        for b in network.Objects:
            if b.name == obj.base:
                EmitMembers(b)
                cur = b
                break
    
    EmitMembers(obj)
    
    print("\t\treturn true;")
    print("\t}")

print("\treturn false;")
print("}\n")

print("")
print("void CDuckLua::GetContentEnumsAsLua()\n{")

i = 1
for net in network.Objects:
    print('\tLuaSetPropInteger(L(), -1, "%s", %d);' % (net.enum_name, i))
    i += 1
print('\tLuaSetPropInteger(L(), -1, "NUM_NETOBJTYPES", %d);' % (i))

i = 1
for net in network.Messages:
    print('\tLuaSetPropInteger(L(), -1, "%s", %d);' % (net.enum_name, i))
    i += 1
print('\tLuaSetPropInteger(L(), -1, "NUM_NETOBJTYPES", %d);' % (i))

i = 0
for img in content.container.images.items:
    print('\tLuaSetPropInteger(L(), -1, "IMAGE_%s", %d);' % (img.name.value.upper(), i))
    i += 1
print('\tLuaSetPropInteger(L(), -1, "NUM_IMAGES", %d);' % (i))

i = 0
for img in content.container.sprites.items:
    print('\tLuaSetPropInteger(L(), -1, "SPRITE_%s", %d);' % (img.name.value.upper(), i))
    i += 1
print('\tLuaSetPropInteger(L(), -1, "NUM_SPRITES", %d);' % (i))

print("}\n")