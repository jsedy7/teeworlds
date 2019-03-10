// main mod file
print('Ducktape version = ' + Duktape.version);

var lastRect = {
    x: 0,
    y: 0,
    w: 0,
    h: 0,
    color: 0
};

var lastClientlocalTime = 0.0;

var game = {
    laserLines: [],
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
        solid: 0
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

function OnUpdate(clientLocalTime)
{
    var delta = clientLocalTime - lastClientlocalTime;
    lastClientlocalTime = clientLocalTime;

    //print("Hello from the dark side! " + someInt);
    TwSetDrawSpace(Teeworlds.DRAW_SPACE_GAME);

    TwRenderSetColorU32(lastRect.color);
    TwRenderQuad(lastRect.x, lastRect.y, lastRect.w, lastRect.h);

    for(var i = 0; i < game.laserLines.length; i++)
    {
        var l = game.laserLines[i];
        if(l.solid == 1) {
            TwRenderSetColorF4(0.66, 0.0, 1.0, 0.7 + Math.sin(clientLocalTime*3.0) * 0.1);
            TwRenderQuad(l.x, l.y, l.w, l.h);
        }
    }
}

function OnMessage(netObj)
{
    //printObj(netObj);

    if(netObj.netID == 0x1) {
        lastRect.x = netObj.x;
        lastRect.y = netObj.y;
        lastRect.w = netObj.w;
        lastRect.h = netObj.h;
        lastRect.color = netObj.color;
    }

    if(netObj.netID == 0x2) {
        var lline = makeLaserLine();
        lline.x = netObj.x * 32;
        lline.y = netObj.y * 32;
        lline.w = netObj.w * 32;
        lline.h = netObj.h * 32;
        lline.solid = netObj.solid;

        var lineID = findLaserLineID(lline);
        if(lineID != -1) {
            game.laserLines[lineID] = lline;
        }
        else {
            game.laserLines.push(lline);
        }

        printObj(lline);

        if(netObj.solid == 0) {
            for(var x = 0; x < netObj.w; x++) {
                for(var y = 0; y < netObj.h; y++) {
                    TwMapSetTileCollisionFlags(netObj.x + x, netObj.y + y, 0); // air
                }
            }
        }
        else {
            for(var x = 0; x < netObj.w; x++) {
                for(var y = 0; y < netObj.h; y++) {
                    TwMapSetTileCollisionFlags(netObj.x + x, netObj.y + y, 1); // solid
                }
            }
        }
    }
}