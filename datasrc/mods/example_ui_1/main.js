// main mod file

var game = {
};

var ui = {
    show_inventory: false,
    dialog_lines: []
};

var localClientID = 0;

function DrawInventory()
{

}

function OnLoaded()
{
    print("Example UI 1");
    TwSetHudPartsShown({
        health: 0,
        armor: 0,
        ammo: 0,
        time: 0,
        killfeed: 0,
        score: 0,
        chat: 0,
        scoreboard: 1,
    });

    //printObj(Teeworlds);
}

function OnUpdate(clientLocalTime, intraTick)
{
   
}

function OnRender(clientLocalTime, intraTick)
{
    if(ui.show_inventory) {
        DrawInventory();
    }

    const uiRect = TwGetUiScreenRect();
    const charCores = TwGetClientCharacterCores();
    const localCore = charCores[localClientID];
    if(!localCore) {
        return;
    }

    var cursorPos = TwGetCursorPosition();
    cursorPos.x -= localCore.pos_x;
    cursorPos.y -= localCore.pos_y;
    //printObj(cursorPos);

    const screenSize = TwGetScreenSize();
    const camera = TwGetCamera();
    var worldPoints = GetWorldViewRect(camera.x, camera.y, screenSize.w/screenSize.h, camera.zoom);
    /*TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME);
    TwRenderSetColorF4(1, 0, 0, 0.5);
    TwRenderQuad(worldPoints.x, worldPoints.y, worldPoints.w, worldPoints.h);*/

    TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_HUD);

    ui.dialog_lines.forEach(function(line) {
        const npcCore = charCores[line.npc_cid];
        if(!npcCore) {
            return;
        }

        const bubbleW = 240;
        const bubbleH = 65;
        var bubbleX = clamp(npcCore.pos_x - bubbleW/2, worldPoints.x, worldPoints.x + worldPoints.w - bubbleW);
        var bubbleY = clamp(npcCore.pos_y - bubbleH - 100, 0, worldPoints.y + worldPoints.h - bubbleH);

        bubbleX = (bubbleX-worldPoints.x)/worldPoints.w * uiRect.w;
        bubbleY = (bubbleY-worldPoints.y)/worldPoints.h * uiRect.h;
        bubbleW = bubbleW/worldPoints.w * uiRect.w;
        bubbleH = bubbleH/worldPoints.h * uiRect.h;

        TwRenderSetColorF4(0, 0, 0, 1);
        TwRenderQuad(bubbleX, bubbleY, bubbleW, bubbleH);

        const npcUiX = (npcCore.pos_x - worldPoints.x) / worldPoints.w * uiRect.w;
        const npcUiY = (npcCore.pos_y - worldPoints.y) / worldPoints.h * uiRect.h;
        var tailTopX1 = clamp(npcUiX - 5, 20 - 5, uiRect.w - 20 - 5);
        var tailTopX2 = clamp(npcUiX + 5, 20 + 5, uiRect.w - 20 + 5);
        var tailX = npcUiX;
        var tailY = npcUiY - 50;

        TwRenderSetFreeform([
            tailTopX1, bubbleY + bubbleH,
            tailTopX2, bubbleY + bubbleH,
            tailX, tailY,
            tailX, tailY,
        ]);
        TwRenderDrawFreeform(0, 0);

        const Margin = 10;
        TwRenderDrawText({
            str: line.text,
            font_size: 13,
            colors: [1, 1, 1, 1],
            rect: [bubbleX+Margin, bubbleY+Margin, bubbleW-Margin, bubbleH-Margin]
        });
    });
}

function OnMessage(packet)
{
    //printObj(packet);
    
    if(packet.tw_msg_id == Teeworlds.NETMSGTYPE_SV_CLIENTINFO)
    {
        if(packet.m_Local == 1) {
            localClientID = packet.m_ClientID; // get the local ID
            // TODO: this is inconvenient...
        }
    }
    else if(packet.mod_id == 0x1) {
        var line = TwNetPacketUnpack(packet, {
            i32_npc_cid: 0,
            str128_text: 0
        });

        printObj(line);
        ui.dialog_lines[line.npc_cid] = line;
    }
}

function OnInput(event)
{
    //printObj(event);
}