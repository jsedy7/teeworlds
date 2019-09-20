// main mod file
print('Ducktape version = ' + Duktape.version);

var lastClientlocalTime = 0.0;

var game = {
    debugRects: [],
    staticBlocks: [],
    dynDisks: [],
    prevDynDisks: [],
};

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

function DrawCircle(pos, radius, color)
{
    const polys = 32;
    var circle = [];
    
    for(var p = 0; p < polys; p++)
    {
        circle.push(Math.cos((p/polys) * 2*Math.PI) * radius, Math.sin((p/polys) * 2*Math.PI) * radius);
        circle.push(0, 0);
        circle.push(Math.cos(((p+1)/polys) * 2*Math.PI) * radius, Math.sin(((p+1)/polys) * 2*Math.PI) * radius);
        circle.push(0, 0);
    }

    TwRenderSetTexture(-1);
    TwRenderSetColorF4(color[0], color[1], color[2], color[3]);
    TwRenderSetFreeform(circle);
    TwRenderDrawFreeform(pos);
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

function OnUpdate(clientLocalTime, intraTick)
{
    var shown = {
        health:     Math.floor(clientLocalTime) % 2,
		armor:      Math.floor(clientLocalTime * 1.1) % 2,
		ammo:       Math.floor(clientLocalTime * 1.2) % 2,
		time:       Math.floor(clientLocalTime * 1.3) % 2,
		killfeed:   Math.floor(clientLocalTime * 1.4) % 2,
		score:      Math.floor(clientLocalTime * 1.5) % 2,
		chat:       Math.floor(clientLocalTime * 1.6) % 2,
    }
    TwSetHudPartsShown(shown);
}

function OnRender(clientLocalTime, intraTick)
{
    var delta = clientLocalTime - lastClientlocalTime;
    lastClientlocalTime = clientLocalTime;

    /*var someInt = Math.floor(clientLocalTime) % 6;
    print("Hello from the dark side! " + someInt);*/
    TwRenderSetDrawSpace(0 /*DRAW_SPACE_GAME*/);

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

    var skinInfo = TwGetStandardSkinInfo();
    
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
            skinInfo.textures[i] = tex[1];
    }

    /*TwRenderSetTeeSkin(skinInfo);
    DrawTeeWeapon(Math.floor(clientLocalTime) % 6, tee.pos_x, tee.pos_y, tee.size);
    TwRenderDrawTeeBodyAndFeet(tee);

    var charList = TwGetClientCharacterPositions();
    charList.forEach(function(char, charId) {
        if(char == null) return;
        TwRenderSetTeeSkin(skinInfo);

        var chartee = {
            size: 64 + 64 * (Math.sin(clientLocalTime * 1.3) * 0.5 + 0.5),
            angle: clientLocalTime,
            pos_x: char.pos_x,
            pos_y: char.pos_y,
            is_walking: false,
            is_grounded: false,
            got_air_jump: true,
            emote: Math.floor(clientLocalTime) % 6
        }

        DrawTeeWeapon(Math.floor(clientLocalTime + charId) % 6, char.pos_x, char.pos_y, chartee.size);
        TwRenderDrawTeeBodyAndFeet(chartee);
    });*/

    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderSetTexture(TwGetModTexture("deadtee.png"));
    TwRenderSetQuadRotation(clientLocalTime * -2.0);
    TwRenderQuad(tee.pos_x, 350, 100, 100);

    
    // draw debug rects
    game.debugRects.forEach(function(r) {
        TwRenderSetTexture(-1);
        TwRenderSetColorU32(r.color);
        TwRenderQuad(r.x, r.y, r.w, r.h);
    });

    // draw hook blocks
    game.staticBlocks.forEach(function(block) {
        TwRenderSetTexture(-1);
        TwRenderSetColorF4(1, 0.5 + (Math.sin(clientLocalTime) * 0.5 + 0.5) * 0.5, 1, 1);
        TwRenderQuad(block.pos_x, block.pos_y, block.width, block.height);
    });

    // draw dynamic disks
    game.predictedDynDisks = TwCollisionGetPredictedDynamicDisks();
    game.dynDisks.forEach(function(disk, diskId) {
        var c = 0.5 + (Math.sin(clientLocalTime) * 0.5 + 0.5) * 0.5;
        var prevDisk = game.prevDynDisks[diskId];
        var pos_x = mix(prevDisk.pos_x, disk.pos_x, intraTick);
        var pos_y = mix(prevDisk.pos_y, disk.pos_y, intraTick);

        DrawCircle([pos_x, pos_y], disk.radius, [c, 0, 0, 1]);
    });

    game.predictedDynDisks.forEach(function(disk, diskId) {
        var c = 0.5 + (Math.sin(clientLocalTime) * 0.5 + 0.5) * 0.5;
        DrawCircle([disk.pos_x, disk.pos_y], disk.radius, [0, c, c, 1]);
    });
    
}

function OnMessage(netObj)
{
    //printObj(netObj);
    
    if(netObj.netID == 0x1) { // DEBUG_RECT
        var rectId = TwUnpackInt32(netObj);
        var rect = {
            x: TwUnpackFloat(netObj),
            y: TwUnpackFloat(netObj),
            w: TwUnpackFloat(netObj),
            h: TwUnpackFloat(netObj),
            color: TwUnpackUint32(netObj),
        };
        game.debugRects[rectId] = rect;
        return;
    }

    if(netObj.netID == 0x3) { // CNetObj_HookBlock
        var blockId = TwUnpackInt32(netObj);
        var block = {
            flags: TwUnpackInt32(netObj),
            pos_x: TwUnpackFloat(netObj),
            pos_y: TwUnpackFloat(netObj),
            vel_x: TwUnpackFloat(netObj),
            vel_y: TwUnpackFloat(netObj),
            width: TwUnpackFloat(netObj),
            height: TwUnpackFloat(netObj),
        };
        game.staticBlocks[blockId] = block;
        TwCollisionSetStaticBlock(blockId, block);
        return;
    }

    if(netObj.netID == 0x4) { // CNetObj_DynamicDisk
        var diskId = TwUnpackInt32(netObj);
        var disk = {
            flags: TwUnpackInt32(netObj),
            pos_x: TwUnpackFloat(netObj),
            pos_y: TwUnpackFloat(netObj),
            vel_x: TwUnpackFloat(netObj),
            vel_y: TwUnpackFloat(netObj),
            radius: TwUnpackFloat(netObj),
            hook_force: TwUnpackFloat(netObj),
        };
        game.prevDynDisks[diskId] = game.dynDisks[diskId];
        game.dynDisks[diskId] = disk;
        TwCollisionSetDynamicDisk(diskId, disk);
        return;
    }
}

/*function OnCharacterCorePreTick(listCharCore, listInput)
{
    //printObj(listCharCore);
    //printObj(listInput);
    var count = listCharCore.length;
    for(var i = 0; i < count; i++)
    {
        if(listCharCore[i] !== null)
        {
        }
    }
    return [ listCharCore, listInput ];
}*/

/*function OnCharacterCorePostTick(listCharCore, listInput)
{
    var count = listCharCore.length;
    for(var i = 0; i < count; i++)
    {
        if(listCharCore[i] !== null)
            listCharCore[i].vel_y -= 10.0;
    }
    return [ listCharCore, listInput ];
}*/