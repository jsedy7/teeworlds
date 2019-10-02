// main mod file

var game = {
    localTime: 0,
    newItems: 0,
    gotItem: false
};

var ui = {
    show_inventory: false,
    dialog_lines: [],
    fkey_was_released: true,
    receivedItemTime: -1,
    backpackIsOpen: false
};

var keyWasReleased = [];

var localClientID = 0;

function DrawBackpackIcon(itemsNotSeenCount)
{
    const texBackpack = TwGetModTexture("backpack");

    const width = 40;
    TwRenderSetTexture(texBackpack);
    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderQuadCentered(25, 25, width, width);

    if(itemsNotSeenCount) {
        const fontSize = 11;
        const text = "" + itemsNotSeenCount;

        const size = TwCalculateTextSize({
            str: text,
            font_size: fontSize,
            line_width: -1
        });

        const Margin = 2;
        const notifW = size.w + Margin * 2;
        const notifH = size.h + Margin * 2;
        const notifX = 5 + width - notifW;
        const notifY = 8 + width - notifH;

        TwRenderSetTexture(-1);
        TwRenderSetColorF4(0.9, 0, 0, 1);
        TwRenderQuad(notifX, notifY, notifW, notifH);

        TwRenderDrawText({
            str: text,
            font_size: fontSize,
            colors: [1, 1, 1, 1],
            rect: [notifX+Margin, notifY+Margin-2]
        });

        const promtX = 28;
        const promtY = 68;
        TwRenderSetTexture(TwGetModTexture("tab_prompt_back"));
        TwRenderSetColorF4(1, 1, 1, 1);
        TwRenderQuadCentered(promtX, promtY, 64, 32);

        TwRenderSetTexture(TwGetModTexture("tab_prompt_front"));
        TwRenderSetColorF4(0.5, 1, 0.7, 1);
        TwRenderQuadCentered(promtX, promtY, 64 - (Math.sin(game.localTime * 5) + 1.0)/2 * 8, 32 - (Math.sin(game.localTime * 5) + 1.0)/2 * 4);
    }
}

function DrawBackpackInventory()
{
    const itemsPerLine = 8;
    const itemsPerColumn = 5;
    const itemWidth = 50;
    const itemSpacing = 8;
    const panelWidth = itemsPerColumn * itemWidth + itemSpacing * (itemsPerColumn+1);
    const panelHeight = itemsPerLine * itemWidth + itemSpacing * (itemsPerLine+1);
    const panelX = 0;
    const panelY = 50;

    TwRenderSetTexture(-1);
    TwRenderSetColorU32(0xFF002255);
    TwRenderQuad(panelX, panelY, panelWidth, panelHeight);

    for(var c = 0; c < itemsPerColumn; c++) {
        for(var l = 0; l < itemsPerLine; l++) {
            TwRenderSetColorU32(0xFF071730);
            TwRenderQuad(panelX + itemSpacing + (itemWidth + itemSpacing) * c, panelY + itemSpacing + (itemWidth + itemSpacing) * l, itemWidth, itemWidth);
        }
    }

    if(game.gotItem) {
        TwRenderSetTexture(TwGetModTexture("potato"));
        const potatoX = panelX + itemSpacing + itemWidth/2;
        const potatoY = panelY + itemSpacing + itemWidth/2;
        const potatoW = itemWidth-10;

        // do shadow
        TwRenderSetColorF4(0, 0, 0, 1);
        TwRenderQuadCentered(potatoX + 2, potatoY + 2, potatoW, potatoW);

        // do potato
        TwRenderSetColorF4(1, 1, 1, 1);
        TwRenderQuadCentered(potatoX, potatoY, potatoW, potatoW);
    }
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
    const frontRect = {
        x: rect.x + (width-frontRectWidth)/2,
        y: rect.y + (width-frontRectWidth)/2,
        w: frontRectWidth,
        h: frontRectWidth
    };

    TwRenderSetTexture(promptFront);
    TwRenderSetColorF4(0.5, 1, 0.7, 1);
    TwRenderQuad(frontRect.x, frontRect.y, frontRect.w, frontRect.h);
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
    });

    const alpha = 1.0 - 0.5 * max(a - 0.5, 0) * 2.0;
    const Margin = 4.0;
    const bgW = size.w + Margin * 2;
    const bgH = size.h + Margin * 2;
    const bgY = posY - a * 50 - 30 - bgH/2;

    TwRenderSetTexture(-1);
    TwRenderSetColorF4(0.95, 0.25, 0.08, alpha);
    TwRenderQuad(posX - bgW/2, bgY, bgW, bgH);

    TwRenderDrawText({
        str: text,
        font_size: fontSize,
        colors: [1, 1, 1, alpha],
        rect: [posX - size.w/2, bgY + Margin]
    });
}

function DrawMouseCursor()
{
    const mousePos = TwGetUiMousePos();
    TwRenderSetTexture(TwGetBaseTexture(Teeworlds.IMAGE_CURSOR));
    TwRenderSetColorF4(1, 1, 1, 1);
    TwRenderQuad(mousePos.x, mousePos.y, 24, 24);
    //printObj(mousePos);
}

function OpenOrCloseBackpack()
{
    if(ui.backpackIsOpen) {
        ui.backpackIsOpen = false;
    }
    else {
        ui.backpackIsOpen = true;
        game.newItems = 0; // mark all new items as seen
    }

    TwSetMenuModeActive(ui.backpackIsOpen);
    TwSetHudPartsShown({
        weapon_cursor: !ui.backpackIsOpen
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

    // draw dialog bubbles
    ui.dialog_lines.forEach(function(line) {
        const npcCore = charCores[line.npc_cid];
        if(!npcCore) {
            return;
        }

        const Margin = 10;
        const bubbleTextW = 160;
        var bubbleW = bubbleTextW;
        const fontSize = 13;

        // calculate text size
        const textSize = TwCalculateTextSize({
            str: line.text,
            font_size: fontSize,
            line_width: bubbleTextW
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

        /*TwRenderSetColorF4(1, 0, 0, 1);
        TwRenderQuad(bubbleX+Margin, bubbleY+Margin, textSize.w, textSize.h);*/
        
        // draw text
        TwRenderDrawText({
            str: line.text,
            font_size: fontSize,
            colors: [1, 1, 1, 1],
            rect: [bubbleX+Margin, bubbleY+Margin, bubbleTextW, textSize.h]
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

    DrawBackpackIcon(game.newItems);

    if(ui.backpackIsOpen) {
        DrawBackpackInventory();
        DrawMouseCursor();
    }
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
        game.newItems = 1;
        game.gotItem = true;
    }
}

function GotKeyPress(event, key)
{
    if(event.pressed && event.key == key) {
        if(keyWasReleased[key] === undefined || keyWasReleased[key]) {
            keyWasReleased[key] = false;
            return true;
        }
    }
    if(event.released && event.key == key) {
        keyWasReleased[key] = true;
    }
    return false;
}
function OnInput(event)
{
    //printObj(event);

    if(GotKeyPress(event, 102)) {
        TwNetSendPacket({
            net_id: 1,
            force_send_now: 1,
        });
    }
    if(GotKeyPress(event, 9)) {
        OpenOrCloseBackpack();
    }
}