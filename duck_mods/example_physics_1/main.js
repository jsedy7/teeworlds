// main mod file

var game = {
    dynDisks: [],
    prevDynDisks: [],
    charCores: [],
};

const testCircle = MakeCircle(20);
var circlMeshes = [];

function MakeCircle(radius)
{
    const polys = 32;
    var circle = new Array(polys * 8);

    for(var p = 0; p < polys; p++)
    {
        circle[p*8]   = Math.cos((p/polys) * 2*Math.PI) * radius;
        circle[p*8+1] = Math.sin((p/polys) * 2*Math.PI) * radius;
        circle[p*8+2] = 0;
        circle[p*8+3] = 0;
        circle[p*8+4] = Math.cos(((p+1)/polys) * 2*Math.PI) * radius;
        circle[p*8+5] = Math.sin(((p+1)/polys) * 2*Math.PI) * radius;
        circle[p*8+6] = 0;
        circle[p*8+7] = 0;
    }

    return circle;
}

function DrawCircle(x, y, radius)
{
    const polys = 16;
    var circle = new Array(polys * 8);
    for(var p = 0; p < polys; p++)
    {
        circle[p*8]   = Math.cos((p/polys) * 2*Math.PI) * radius;
        circle[p*8+1] = Math.sin((p/polys) * 2*Math.PI) * radius;
        circle[p*8+2] = 0;
        circle[p*8+3] = 0;
        circle[p*8+4] = Math.cos(((p+1)/polys) * 2*Math.PI) * radius;
        circle[p*8+5] = Math.sin(((p+1)/polys) * 2*Math.PI) * radius;
        circle[p*8+6] = 0;
        circle[p*8+7] = 0;
    }

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
        scoreboard: 1,
    });

    //printObj(Teeworlds);
}

function OnUpdate(clientLocalTime, intraTick)
{
    
}

function OnRender(clientLocalTime, intraTick)
{
    TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME_FOREGROUND);

    var cores = TwPhysGetCores();
    var joints = TwPhysGetJoints();

    for(var i = 0; i < cores.length; i++) {
        const core = cores[i];
        TwRenderSetColorF4(1, 1, 1, 1);
        TwRenderDrawCircle(core.x, core.y, core.radius);

        TwRenderSetTexture(-1);
        TwRenderSetColorF4(1, 0, 1, 0.5);
        TwRenderQuadCentered(core.x, core.y, core.radius * 2, core.radius * 2);
    }

    joints.forEach(function(joint) {
        if(joint.core1_id === null || joint.core2_id === null)
            return;

        var core1 = cores[joint.core1_id];
        var core2 = cores[joint.core2_id];

        TwRenderSetColorF4(0, 0.2, 1, 0.85);
        TwRenderDrawLine(core1.x, core1.y, core2.x, core2.y, 10);
    });

    /*TwRenderSetTexture(-1);
    TwRenderSetColorF4(1, 1, 1, 1);

    for(var y = 0; y < 30; y++) {
        for(var x = 0; x < 30; x++) {
            var rx = x * 50;
            var ry = y * 50;
            var radius = 20;
            TwRenderDrawCircle(rx, ry, radius);
        }
    }

    TwRenderSetColorF4(1, 0, 1, 0.5);

    for(var y = 0; y < 30; y++) {
        for(var x = 0; x < 30; x++) {
            var rx = x * 50;
            var ry = y * 50;
            var radius = 20;
            TwRenderQuadCentered(rx, ry, radius * 2, radius * 2);
        }
    }*/
}

function OnMessage(packet)
{
}

function OnInput(event)
{
    //printObj(event);
}