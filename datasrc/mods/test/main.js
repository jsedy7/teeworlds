// main mod file
print('Ducktape version = ' + Duktape.version);

var lastClientlocalTime = 0.0;

OnLoad();


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

function fmod(a, b)
{
    return Number((a - (Math.floor(a / b) * b)).toPrecision(8));
}

function OnLoad()
{
    printObj(TwGetWeaponSpec(1));
}

function DrawTeeWeapon(weapId, x, y, teeSize)
{
    var angle = lastClientlocalTime;
    var dir = TwDirectionFromAngle(angle);
    var spec = TwGetWeaponSpec(weapId);
    var ss = spec.sprite_body_subset;
    var scale = spec.sprite_body_scale;
    var teeScale = teeSize/64;
    var quadW = scale.w * spec.visual_size * teeScale;
    var quadH = scale.h * spec.visual_size * teeScale;
    var offX = spec.offset_x * teeScale
    var offY = spec.offset_y * teeScale

    var weapX = x - quadW / 2 + (dir.x * offX);
    var weapY = y - quadH / 2 + (dir.y * offX) + dir.y * offY;

    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderSetTexture(spec.sprite_body_texid);
    if(dir.x > 0)
        TwRenderSetQuadSubSet(ss.x1, ss.y1, ss.x2, ss.y2);
    else
        TwRenderSetQuadSubSet(ss.x1, ss.y2, ss.x2, ss.y1); // FLIP Y

    TwRenderSetQuadRotation(angle);
    TwRenderQuad(weapX, weapY, quadW, quadH);

    // tee hand
    var hand = {
        size: teeSize,
        angle_dir: angle,
        angle_off: -(Math.PI/2),
        pos_x: weapX + quadW/2,
        pos_y: weapY + quadH/2,
    };

    if(weapId == 1) {
        hand.off_x = -15 * teeScale;
        hand.off_y = 4 * teeScale;
        TwRenderDrawTeeHand(hand);
    }
    else if(weapId == 2) {
        hand.off_x = -5 * teeScale;
        hand.off_y = 4 * teeScale;
        TwRenderDrawTeeHand(hand);
    }
    else if(weapId == 3) {
        hand.off_x = -4 * teeScale;
        hand.off_y = 7 * teeScale;
        TwRenderDrawTeeHand(hand);
    }
}

function OnUpdate(clientLocalTime)
{
    var delta = clientLocalTime - lastClientlocalTime;
    lastClientlocalTime = clientLocalTime;

    //print("Hello from the dark side! " + someInt);
    TwSetDrawSpace(Teeworlds.DRAW_SPACE_GAME);

    var tee = {
        size: 128,
        angle: clientLocalTime,
        pos_x: 150 + 500.0 * (Math.sin(clientLocalTime) * 0.5 + 0.5),
        pos_y: 305,
        is_walking: false,
        is_grounded: false,
        got_air_jump: true,
        emote: Math.floor(clientLocalTime) % 6
    }
    
    DrawTeeWeapon(Math.floor(clientLocalTime) % 6, tee.pos_x, tee.pos_y, tee.size);
    TwRenderDrawTeeBodyAndFeet(tee);
}

function OnMessage(netObj)
{
    //printObj(netObj);
}