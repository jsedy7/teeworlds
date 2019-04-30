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

function objToStr(obj)
{
    if(Array.isArray(obj)) {
        var str = "[ ";
        var arrayLength = obj.length;
        for (var i = 0; i < arrayLength; i++) {
            str += objToStr(obj[i]) + ", ";
        }
        str += "]";
        return str;
    }

    if((typeof obj === "object") && (obj !== null)) {
        var str = "{ ";
        for(var key in obj) {
            if(obj.hasOwnProperty(key)) {
                str += key + ": " + objToStr(obj[key]) + ", ";
            }
        }
        str += "}";
        return str;
    }

    return "" + obj;
}

function printObj(obj)
{
    print(objToStr(obj));
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

    var weapX = x + (dir.x * offX);
    var weapY = y + (dir.y * offX) + dir.y * offY;

    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderSetTexture(spec.sprite_body_texid);
    if(dir.x > 0)
        TwRenderSetQuadSubSet(ss.x1, ss.y1, ss.x2, ss.y2);
    else
        TwRenderSetQuadSubSet(ss.x1, ss.y2, ss.x2, ss.y1); // FLIP Y

    TwRenderSetQuadRotation(angle);
    TwRenderQuadCentered(weapX, weapY, quadW, quadH);

    // tee hand
    var hand = {
        size: teeSize,
        angle_dir: angle,
        angle_off: -(Math.PI/2),
        pos_x: weapX,
        pos_y: weapY,
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
        size: 64 + 64 * (Math.sin(clientLocalTime * 1.3) * 0.5 + 0.5),
        angle: clientLocalTime,
        pos_x: 150 + 500.0 * (Math.sin(clientLocalTime) * 0.5 + 0.5),
        pos_y: 305,
        is_walking: false,
        is_grounded: false,
        got_air_jump: true,
        emote: Math.floor(clientLocalTime) % 6
    }

    var skinInfo = TwGetClientSkinInfo(0);
    
    var partNames = [
        [
            "bear",
            "kitty",
            "standard"
        ],
        [
            "bear",
            "cammo1",
            "cammo2",
            "cammostripes",
            "donny",
            "duodonny",
            "saddo",
            "stripe",
            "stripes",
            "toptri",
            "twintri",
            "uppy",
            "warpaint",
            "whisker",
        ],
        [
            "hair",
            "twinbopp",
            "unibop",
        ],
        [
            "standard"
        ],
        [
            "standard"
        ],
        [
            "standard"
        ]
    ];

    for(var i = 0; i < 6; i++) {
        skinInfo.colors[i].r = Math.cos(clientLocalTime + i*i) * 0.5 + 0.5;
        skinInfo.colors[i].g = Math.cos(clientLocalTime * 2 + i*i) * 0.5 + 0.5;
        skinInfo.colors[i].b = Math.cos(clientLocalTime * 3 + i*i) * 0.5 + 0.5;
        var tex = TwGetSkinPartTexture(i, partNames[i][Math.floor(clientLocalTime * (i/3.0+1.0)) % partNames[i].length]);

        if(tex != null)
            skinInfo.textures[i] = tex[0];
    }

    TwRenderSetTeeSkin(skinInfo);

    DrawTeeWeapon(Math.floor(clientLocalTime) % 6, tee.pos_x, tee.pos_y, tee.size);
    
    TwRenderDrawTeeBodyAndFeet(tee);

    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderSetTexture(TwGetModTexture("deadtee.png"));
    TwRenderSetQuadRotation(clientLocalTime * -20.0);
    TwRenderQuad(tee.pos_x, 350, 100, 100);
}

function OnMessage(netObj)
{
    //printObj(netObj);
}