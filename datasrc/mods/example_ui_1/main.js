// main mod file

var game = {
    localTime:0
};

var ui = {
    show_inventory: false,
    dialog_lines: [],
    fkey_was_released: true,
    receivedItemTime: -1
};

var localClientID = 0;

function DrawInventory()
{

}

function DrawInteractPrompt(clientLocalTime, posX, posY, width)
{
    const promptBack = TwGetModTexture("key_prompt_back");
    const promptFront = TwGetModTexture("key_prompt_front");

    const rect = {
        x: posX - width/2,
        y: posY - width/2,
        w: width,
        h: width,
    };

    TwRenderSetTexture(promptBack);
    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h);

    const frontRectWidth = width - (Math.sin(clientLocalTime * 5) + 1.0)/2 * 8;
    //const frontRectWidth = width;
    const frontRect = {
        x: rect.x + (width-frontRectWidth)/2,
        y: rect.y + (width-frontRectWidth)/2,
        w: frontRectWidth,
        h: frontRectWidth
    };

    TwRenderSetTexture(promptFront);
    TwRenderSetColorF4(0.5, 1, 0.7, 1);
    TwRenderQuad(frontRect.x, frontRect.y, frontRect.w, frontRect.h);

    /*const fontSize = frontRectWidth - 25;
    const fRect = {
        x: rect.x + rect.w/2 - fontSize/3,
        y: rect.y + rect.h/2 - fontSize/1.5,
        w: rect.w,
        h: rect.h
    };

    TwRenderSetTexture(-1);
    TwRenderSetColorF4(1, 0, 0, 0.5);
    TwRenderQuad(fRect.x, fRect.y, fRect.w, fRect.h);

    TwRenderDrawText({
        str: "F",
        font_size: fontSize,
        colors: [0, 0, 0, 1],
        rect: [fRect.x, fRect.y, fRect.w, fRect.h]
    });*/
}

function DrawItemNotification(posX, posY, startTime, clientLocalTime)
{
    if(startTime < 0)
        return;

    const a = (clientLocalTime - startTime) / 3.0;
    if(a < 0.0 || a > 1.0)
        return;

    const fontSize = 12;
    const text = "+ Item received";

    const size = TwCalculateTextSize({
        str: text,
        font_size: fontSize,
        line_width: -1
    });

    TwRenderDrawText({
        str: text,
        font_size: fontSize,
        colors: [1, 1, 1, 1.0 - a * 0.5],
        rect: [posX - size.w/2, posY - a * 50 - 30, 10000, 500]
    });
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
    game.localTime = clientLocalTime;

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

    var displayInteract = false;

    // draw dialog bubbles
    ui.dialog_lines.forEach(function(line) {
        const npcCore = charCores[line.npc_cid];
        if(!npcCore) {
            return;
        }

        // close enough to npc to interact, display prompt
        if(distance({ x: localCore.pos_x, y: localCore.pos_y }, { x: npcCore.pos_x, y: npcCore.pos_x }) < 400) {
            displayInteract = true;
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
        var tailTopY = bubbleY + bubbleH;
        var tailX = npcUiX;
        var tailY = npcUiY - 50;

        TwRenderSetFreeform([
            tailTopX1, tailTopY,
            tailTopX2, tailTopY,
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

        // close enough to npc to interact, display prompt
        if(distance({ x: localCore.pos_x, y: localCore.pos_y }, { x: npcCore.pos_x, y: npcCore.pos_y }) < 400) {
            DrawInteractPrompt(clientLocalTime, npcUiX, tailTopY + 10, 30);
        }
    });

    // get npc pos in hud space
    const localCoreUiX = (localCore.pos_x - worldViewRect.x) / worldViewRect.w * uiRect.w;
    const localCoreUiY = (localCore.pos_y - worldViewRect.y) / worldViewRect.h * uiRect.h;
    DrawItemNotification(localCoreUiX, localCoreUiY, ui.receivedItemTime, clientLocalTime);
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
    else if(packet.mod_id == 0x2) {
        print("received item");
        ui.receivedItemTime = game.localTime;
    }
}

function OnInput(event)
{
    //printObj(event);

    if(event.pressed && event.key == 102) {
        if(ui.fkey_was_released) {
            TwNetSendPacket({
                net_id: 1,
                force_send_now: 1,
            });
            ui.fkey_was_released = false;
        }
    }
    if(event.released && event.key == 102) {
        ui.fkey_was_released = true;
    }
}