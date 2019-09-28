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
    var worldViewRect = GetWorldViewRect(camera.x, camera.y, screenSize.w/screenSize.h, camera.zoom);
    /*TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME);
    TwRenderSetColorF4(1, 0, 0, 0.5);
    TwRenderQuad(worldViewRect.x, worldViewRect.y, worldViewRect.w, worldViewRect.h);*/

    TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_HUD);

    // draw dialog bubbles
    ui.dialog_lines.forEach(function(line) {
        const npcCore = charCores[line.npc_cid];
        if(!npcCore) {
            return;
        }

        const Margin = 10;
        var bubbleW = 160;

        // calculate text size
        const textSize = TwCalculateTextSize({
            str: line.text,
            font_size: 13,
            line_width: bubbleW
        });

        var bubbleH = textSize.h + 2 + Margin*2;
        bubbleW += Margin*2;

        // get npc pos in hud space
        const npcUiX = (npcCore.pos_x - worldViewRect.x) / worldViewRect.w * uiRect.w;
        const npcUiY = (npcCore.pos_y - worldViewRect.y) / worldViewRect.h * uiRect.h;

        // clamp bubble position inside view
        var bubbleX = clamp(npcUiX - bubbleW/2, 0, uiRect.w - bubbleW);
        var bubbleY = clamp(npcUiY - bubbleH - 70, 0, uiRect.h - bubbleH);

        // draw rect background
        TwRenderSetColorF4(0, 0, 0, 1);
        TwRenderQuad(bubbleX, bubbleY, bubbleW, bubbleH);
        
        // draw tail
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
        
        // draw text
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