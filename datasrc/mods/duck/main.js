// main mod file
print('Ducktape version = ' + Duktape.version);

var lastRectPrev = {};
var lastRect = {
    x: 0,
    y: 0,
    w: 0,
    h: 0,
};

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

function OnUpdate(clientLocalTime)
{
    //print("Hello from the dark side! " + someInt);
    TwSetDrawSpace(Teeworlds.DRAW_SPACE_GAME);
    var blend = Teeworlds.intraTick;
    TwRenderQuad(mix(lastRectPrev.x, lastRect.x, blend), mix(lastRectPrev.y, lastRect.y, blend), lastRect.w, lastRect.h);
}

function OnMessage(netObj)
{
    //printObj(netObj);

    if(netObj.netID == 0x1) {
        lastRectPrev = clone(lastRect);
        lastRect.x = netObj.x;
        lastRect.y = netObj.y;
        lastRect.w = netObj.w;
        lastRect.h = netObj.h;
    }
}