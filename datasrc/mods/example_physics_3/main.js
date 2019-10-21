// main mod file

var game = {
    bees: [],
};

var isDebug = false;

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

    const wingAnimSS = [
        { x1: 0.5, y1: 0.0 },
        { x1: 0.5, y1: 0.5 },
        { x1: 0.5, y1: 0.0 },
        { x1: 0.0, y1: 0.5 },
    ];

    game.bees.forEach(function(bee) {
        var core1 = cores[bee.core1ID];
        var core2 = cores[bee.core2ID];

        TwRenderSetTexture(TwGetModTexture("bee_body"));
        TwRenderSetColorF4(1, 1, 1, 1);

        var beeDir = vec2Normalize(vec2Sub(core2, core1));
        var beeAngle = vec2Angle(beeDir);

        // tail
        var tailDir = vec2Normalize(vec2Sub(vec2(core2.x, core2.y - 20 * beeDir.x), core1));
        var tailAngle = vec2Angle(tailDir);
        var tailOffset = vec2Rotate(vec2(-8, 0), tailAngle);
        var tailSizeVar = (Math.sin(clientLocalTime * 5) * 2) * 0.5 * 4;

        TwRenderSetQuadRotation(tailAngle);
        TwRenderSetQuadSubSet(0.5, 0, 1, 0.5);
        TwRenderQuadCentered(core2.x + tailOffset.x, core2.y + tailOffset.y, 128 + tailSizeVar, 128 + tailSizeVar);

        // middle
        TwRenderSetQuadRotation(beeAngle);
        TwRenderSetQuadSubSet(0.5, 0.5, 1, 1);
        TwRenderQuadCentered(core1.x, core1.y, 128, 128);

        // head
        var headOffset = vec2Rotate(vec2(-72, 0), beeAngle);
        var headAngleVar = Math.sin(clientLocalTime * 3) * Math.PI * 0.05;
        TwRenderSetQuadRotation(beeAngle + headAngleVar);
        TwRenderSetQuadSubSet(0, 0, 0.5, 0.5);
        TwRenderQuadCentered(core1.x + headOffset.x, core1.y + headOffset.y, 128, 128);

        // wing
        var wingOffset = vec2Rotate(vec2(15, -55), beeAngle);
        var wingSize = 150;
        var wingAnim = wingAnimSS[Math.floor(clientLocalTime * 20) % 4];
        TwRenderSetTexture(TwGetModTexture("bee_wing_anim"));
        TwRenderSetQuadSubSet(wingAnim.x1, wingAnim.y1, wingAnim.x1 + 0.5, wingAnim.y1 + 0.5);
        TwRenderSetQuadRotation(beeAngle);
        TwRenderQuadCentered(core1.x + wingOffset.x, core1.y + wingOffset.y, wingSize, wingSize);
    });

    if(isDebug) {
        var joints = TwPhysGetJoints();

        TwRenderSetTexture(-1);
        
        for(var i = 0; i < cores.length; i++) {
            const core = cores[i];
            TwRenderSetColorF4(1, 1, 1, 1);
            TwRenderDrawCircle(core.x, core.y, core.radius);

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
    }
}

function OnMessage(packet)
{
    /*if(packet.mod_id) {
        printObj(packet);
    }*/
}

function OnSnap(packet, snapID)
{
    if(packet.mod_id == 0x1) {
        var bee = TwNetPacketUnpack(packet, {
            i32_core1ID: 0,
            i32_core2ID: 0,
            i32_health:   0,
        });

        game.bees[snapID] = bee;
    }
}

function OnInput(event)
{
    //printObj(event);

    if(event.key == 112 && event.released == 1) {
        isDebug = !isDebug;
    }
}