// main mod file

var game = {
    dynDisks: [],
    prevDynDisks: [],
    charCores: [],
};

function DrawCircle(x, y, radius, color)
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
    TwRenderDrawFreeform(x, y);
}

function OnLoaded()
{
    print("Example Physics 1");
    TwSetHudPartsShown({
        health: 0,
        armor: 0,
        ammo: 0,
        time: 0,
        killfeed: 0,
        score: 0,
        chat: 1,
        scoreboard: 0,
    });

    //printObj(Teeworlds);
}

function OnUpdate(clientLocalTime, intraTick)
{
   
}

function OnRender(clientLocalTime, intraTick)
{
    TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME_FOREGROUND);
    
    // draw dynamic disks
    game.predictedDynDisks = TwCollisionGetPredictedDynamicDisks();
    game.dynDisks.forEach(function(disk, diskId) {
        var c = 0.5 + (Math.sin(clientLocalTime) * 0.5 + 0.5) * 0.5;
        var prevDisk = game.prevDynDisks[diskId];
        var pos_x = mix(prevDisk.pos_x, disk.pos_x, intraTick);
        var pos_y = mix(prevDisk.pos_y, disk.pos_y, intraTick);

        DrawCircle(pos_x, pos_y, disk.radius, [c, 0, 0, 1]);
    });

    game.predictedDynDisks.forEach(function(disk, diskId) {
        var c = 0.5 + (Math.sin(clientLocalTime) * 0.5 + 0.5) * 0.5;
        DrawCircle(disk.pos_x, disk.pos_y, disk.radius, [0, c, c, 1]);
    });

    game.charCores.forEach(function(core, coreID) {
        DrawCircle(core.x, core.y, 28, [1, 0, 1, 1]);
    });

    var cores = TwGetDuckCores();

    cores.forEach(function(core, coreID) {
        DrawCircle(core.x, core.y, 28, [1, 0, 1, 1]);
    });
}

function OnMessage(packet)
{
    if(packet.mod_id == 0x1) {
        //printObj(packet);

        var disk = TwNetPacketUnpack(packet, {
            i32_diskID: 0,
            i32_flags:   0,
            float_pos_x: 0,
            float_pos_y: 0,
            float_vel_x: 0,
            float_vel_y: 0,
            float_radius: 0,
            float_hook_force:0,
        });

        var diskId = disk.diskID;
        game.prevDynDisks[diskId] = game.dynDisks[diskId];
        game.dynDisks[diskId] = disk;
        TwCollisionSetDynamicDisk(diskId, disk);
    }
    else if(packet.mod_id == 0x2) {
        //printObj(packet);

        var charCore = TwNetPacketUnpack(packet, {
            i32_ID: 0,
            float_x: 0,
            float_y: 0,
            float_vel_x: 0,
            float_vel_y: 0,
        });

        game.charCores[charCore.ID] = charCore;
    }
}

function OnInput(event)
{
    //printObj(event);
}