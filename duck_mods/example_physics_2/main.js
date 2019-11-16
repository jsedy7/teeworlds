// main mod file
print('Ducktape version = ' + Duktape.version);

var lastClientlocalTime = 0.0;

var game = {
    laserLines: [],
    debugRects: []
};


// ----  FUNCTIONS -----

function clone(obj)
{
    var objClone = {};
    for(var key in obj) {
        if(obj.hasOwnProperty(key)) {
            objClone[key] = obj[key];
        }
    }
    return objClone;
}

function copy(destObj, srcObj)
{
    for(var key in srcObj) {
        if(srcObj.hasOwnProperty(key)) {
            destObj[key] = srcObj[key];
        }
    }
}

function printObj(obj)
{
    var str = "{ ";
    for(var key in obj) {
        if(obj.hasOwnProperty(key)) {
            str += key + ": " + obj[key] + ", ";
        }
    }
    str += "}"
    print(str);
}

function printRawBuffer(buff)
{
    var str = "[";
    for(var i = 0; i < buff.length; i +=1) {
        str += "0x"+ buff[i].toString(16) + ",";
    }
    str += "]"
    print(str);
}

function mix(a, b, amount)
{
    return a + (b-a) * amount;
}

function makeLaserLine()
{
    return {
        x: 0,
        y: 0,
        w: 0,
        h: 0,
        solid: 0,
        hookable: 0,
    };
}

function arelaserLinesEqual(l1, l2)
{
    return l1.x == l2.x && l1.y == l2.y && l1.w == l2.w && l1.h == l2.h;
}

function findLaserLineID(line)
{
    for(var i = 0; i < game.laserLines.length; i++)
    {
        if(arelaserLinesEqual(game.laserLines[i], line))
        {
            return i;
        }
    }

    return -1;
}

function OnRender(clientLocalTime)
{
    var delta = clientLocalTime - lastClientlocalTime;
    lastClientlocalTime = clientLocalTime;

    TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME_FOREGROUND);

    game.debugRects.forEach(function(r) {
        TwRenderSetColorU32(r.color);
        TwRenderQuad(r.x, r.y, r.w, r.h);
    });

    for(var i = 0; i < game.laserLines.length; i++)
    {
        var l = game.laserLines[i];
        if(l.solid == 1) {
            var alpha = 0.7 + Math.sin(clientLocalTime*3.0) * 0.1;
            if(l.hookable)
                TwRenderSetColorF4(0.84, 0.34, 1.0, alpha);
            else
                TwRenderSetColorF4(1.0, 0.84, 0.34, alpha);
            TwRenderQuad(l.x, l.y, l.w, l.h);

            TwRenderSetColorF4(1.0, 1.0, 1.0, 0.5);
            TwRenderQuad(l.x, l.y, l.w, 2.0);
            TwRenderQuad(l.x, l.y + l.h - 2.0, l.w, 2.0);
        }
    }
}

function OnMessage(packet)
{
    //printObj(netObj);

    if(packet.mod_id == 0x1) { // DEBUG_RECT
        var rect = TwNetPacketUnpack(packet, {
            i32_id: 0,
            float_x: 0,
            float_y: 0,
            float_w: 0,
            float_h: 0,
            u32_color: 0,
        });

        game.debugRects[rect.id] = rect;
    }

    if(packet.mod_id == 0x2) { // MAP_RECT_SET_SOLID
        var rect = TwNetPacketUnpack(packet, {
            i32_solid: 0,
            i32_hookable: 0,
            i32_tx: 0,
            i32_ty: 0,
            i32_tw: 0,
            i32_th: 0,
        });

        var lline = makeLaserLine();
        lline.solid = rect.solid;
        lline.hookable = rect.hookable;
        lline.x = rect.tx * 32;
        lline.y = rect.ty * 32;
        lline.w = rect.tw * 32;
        lline.h = rect.th * 32;

        var lineID = findLaserLineID(lline);
        if(lineID != -1) {
            game.laserLines[lineID] = lline;
        }
        else {
            game.laserLines.push(lline);
        }

        //printObj(lline);

        if(rect.solid == 0) {
            for(var x = 0; x < rect.tw; x++) {
                for(var y = 0; y < rect.th; y++) {
                    TwMapSetTileCollisionFlags(rect.tx + x, rect.ty + y, 0); // air
                }
            }
        }
        else {
            for(var x = 0; x < rect.tw; x++) {
                for(var y = 0; y < rect.th; y++) {
                    TwMapSetTileCollisionFlags(rect.tx + x, rect.ty + y, 1 | (4 * !rect.hookable)); // solid
                }
            }
        }
    }
}