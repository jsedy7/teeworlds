-- main mod file
require("utils.lua")

local game = {
    localTime = 0,
    newItems = 0,
    gotItem = false,
}

local ui = {
    receivedItemTime = -1,
    backpackIsOpen = false,
}

function CreateNPC()
    return {
        dialog_line = nil,
        question = nil,
        hoveredID = nil
    }
end

local npcs = {}

local keyWasReleased = {}
local localClientID = 0

function DrawBackpackIcon(itemsNotSeenCount)
    local texBackpack = TwGetModTexture("backpack")

    local width = 40
    TwRenderSetTexture(texBackpack)
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderQuadCentered(25, 25, width, width)

    if itemsNotSeenCount then
        local fontSize = 11
        local text = "" .. itemsNotSeenCount

        local size = TwCalculateTextSize({
            text = text,
            font_size = fontSize,
            line_width = -1
        })

        local Margin = 2
        local notifW = size.w + Margin * 2
        local notifH = size.h + Margin * 2
        local notifX = 5 + width - notifW
        local notifY = 8 + width - notifH

        TwRenderSetTexture(-1)
        TwRenderSetColorF4(0.9, 0, 0, 1)
        TwRenderQuad(notifX, notifY, notifW, notifH)

        TwRenderDrawText({
            text = text,
            font_size = fontSize,
            color = {1, 1, 1, 1},
            rect = {notifX+Margin, notifY+Margin-2}
        })

        local promtX = 28
        local promtY = 68
        TwRenderSetTexture(TwGetModTexture("tab_prompt_back"))
        TwRenderSetColorF4(1, 1, 1, 1)
        TwRenderQuadCentered(promtX, promtY, 64, 32)

        TwRenderSetTexture(TwGetModTexture("tab_prompt_front"))
        TwRenderSetColorF4(0.5, 1, 0.7, 1)
        TwRenderQuadCentered(promtX, promtY, 64 - (sin(game.localTime * 5) + 1.0)/2 * 8, 32 - (sin(game.localTime * 5) + 1.0)/2 * 4)
    end
end

function DrawBackpackInventory()
    local itemsPerLine = 8
    local itemsPerColumn = 5
    local itemWidth = 50
    local itemSpacing = 8
    local panelWidth = itemsPerColumn * itemWidth + itemSpacing * (itemsPerColumn+1)
    local panelHeight = itemsPerLine * itemWidth + itemSpacing * (itemsPerLine+1)
    local panelX = 0
    local panelY = 50

    local mousePos = TwGetUiMousePos()

    TwRenderSetTexture(-1)
    TwRenderSetColorU32(0xFF002255)
    TwRenderQuad(panelX, panelY, panelWidth, panelHeight)

    for c = 0, itemsPerColumn-1, 1 do
        for l = 0, itemsPerLine-1, 1 do
            TwRenderSetColorU32(0xFF071730)
            TwRenderQuad(panelX + itemSpacing + (itemWidth + itemSpacing) * c, panelY + itemSpacing + (itemWidth + itemSpacing) * l, itemWidth, itemWidth)
        end
    end

    if game.gotItem then
        TwRenderSetTexture(TwGetModTexture("potato"))
        local potatoX = panelX + itemSpacing + itemWidth/2
        local potatoY = panelY + itemSpacing + itemWidth/2
        local potatoW = itemWidth-10

        local potatoRect = {
            x = potatoX - itemWidth/2,
            y = potatoY - itemWidth/2,
            w = itemWidth,
            h = itemWidth,
        }
    
        local hovered = IsPointInsideRect(mousePos, potatoRect)

        if hovered then
            potatoW = potatoW + 3 + 3 * (sin(game.localTime * 5) + 1.0) / 2
        end

        -- do shadow
        TwRenderSetColorF4(0, 0, 0, 1)
        TwRenderQuadCentered(potatoX + 2, potatoY + 2, potatoW, potatoW)

        -- do potato
        TwRenderSetColorF4(1, 1, 1, 1)
        TwRenderQuadCentered(potatoX, potatoY, potatoW, potatoW)

        -- if hovered, draw item description
        if hovered then
            local descRect = {
                x = potatoRect.x + itemWidth + itemSpacing / 2,
                y = potatoRect.y,
                w = 180,
                h = 200
            }

            DrawBorderRect(descRect, 4, {0, 0, 0, 1}, {0.4, 0.4, 0.4, 1})
            DrawBorderRect(potatoRect, 2, {0, 0, 0, 0}, {1, 0, 0, 1}) -- outline

            local headerHeight = 30
            DrawRect({
                x = descRect.x,
                y = descRect.y,
                w = descRect.w,
                h = headerHeight,
            }, {0.4, 0.4, 0.4, 1})

            local headerTextSize = TwCalculateTextSize({
                text = "Potato",
                font_size = 13,
            })

            TwRenderDrawText({
                text = "Potato",
                font_size = 13,
                color = {1, 1, 1, 1},
                rect = {descRect.x + descRect.w/2 - headerTextSize.w/2, descRect.y + headerHeight/2 - headerTextSize.h/2}
            })

            TwRenderDrawText({
                text = "+10 health points on consumption when cooked",
                font_size = 10,
                color = {0.3, 1, 0.2, 1},
                rect = {descRect.x + 10, descRect.y + headerHeight + 5, descRect.w - 20}
            })

            TwRenderDrawText({
                text = "The potato is considered as the fourth most important crop behind the corn, wheat, and rice. The average American eats 140 pounds of potatoes per year.",
                font_size = 10,
                color = {1, 1, 1, 1},
                rect = {descRect.x + 10, descRect.y + headerHeight + 50, descRect.w - 20}
            })

            local raritySize = TwCalculateTextSize({
                text = "common",
                font_size = 10,
            })

            TwRenderDrawText({
                text = "common",
                font_size = 10,
                color = {0.4, 0.4, 0.4, 1},
                rect = {descRect.x + descRect.w/2 - raritySize.w/2, descRect.y + descRect.h - raritySize.h - 5}
            })
        end
    end
end

function DrawInteractPrompt(clientLocalTime, posX, posY, width)
    local promptBack = TwGetModTexture("key_prompt_back")
    local promptFront = TwGetModTexture("key_prompt_front")

    local rect = {
        x = posX - width/2,
        y = posY - width/2,
        w = width,
        h = width,
    }

    TwRenderSetTexture(promptBack)
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h)

    local frontRectWidth = width - (sin(clientLocalTime * 5) + 1.0)/2 * 8
    local frontRect = {
        x = rect.x + (width-frontRectWidth)/2,
        y = rect.y + (width-frontRectWidth)/2,
        w = frontRectWidth,
        h = frontRectWidth
    }

    TwRenderSetTexture(promptFront)
    TwRenderSetColorF4(0.5, 1, 0.7, 1)
    TwRenderQuad(frontRect.x, frontRect.y, frontRect.w, frontRect.h)
end

function DrawItemNotification(posX, posY, startTime, clientLocalTime)
    if startTime < 0 then
        return
    end

    local a = (clientLocalTime - startTime) / 3.0
    if a < 0.0 or a > 1.0 then
        return
    end

    local fontSize = 12
    local text = "+ Item received"

    local size = TwCalculateTextSize({
        text = text,
        font_size = fontSize,
    })

    local alpha = 1.0 - 0.5 * max(a - 0.5, 0) * 2.0
    local Margin = 4.0
    local bgW = size.w + Margin * 2
    local bgH = size.h + Margin * 2
    local bgY = posY - a * 50 - 30 - bgH/2

    TwRenderSetTexture(-1)
    TwRenderSetColorF4(0.95, 0.25, 0.08, alpha)
    TwRenderQuad(posX - bgW/2, bgY, bgW, bgH)

    TwRenderDrawText({
        text = text,
        font_size = fontSize,
        color = {1, 1, 1, alpha},
        rect = {posX - size.w/2, bgY + Margin}
    })
end

function DrawMouseCursor()
    local mousePos = TwGetUiMousePos()
    TwRenderSetTexture(TwGetBaseTexture(Teeworlds.IMAGE_CURSOR))
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderQuad(mousePos.x, mousePos.y, 24, 24)
    --PrintTable(mousePos)
end

function OpenOrCloseBackpack()
    if ui.backpackIsOpen then
        ui.backpackIsOpen = false
    else
        ui.backpackIsOpen = true
        game.newItems = 0 -- mark all new items as seen
    end

    TwSetMenuModeActive(ui.backpackIsOpen)
    TwSetHudPartsShown({
        weapon_cursor = not ui.backpackIsOpen
    })
end

function OnLoad()
    print("Example UI 1")
    TwSetHudPartsShown({
        health = false,
        armor = false,
        ammo = false,
        time = false,
        killfeed = false,
        score = false,
        chat = true,
        scoreboard = false,
    })

    --PrintTable(Teeworlds)
end

function OnUpdate(clientLocalTime, intraTick)
end

function OnRender(clientLocalTime, intraTick)
    game.localTime = clientLocalTime

    local uiRect = TwGetUiScreenRect()
    local charCores = TwGetClientCharacterCores()
    local localCore = charCores[localClientID]
    if not localCore then
        return
    end

    local cursorPos = TwGetCursorPosition()
    cursorPos.x = cursorPos.x - localCore.pos_x
    cursorPos.y = cursorPos.y - localCore.pos_y
    --PrintTable(cursorPos)

    local screenSize = TwGetScreenSize()
    local camera = TwGetCamera()
    local worldViewRect = GetWorldViewRect(camera.x, camera.y, screenSize.w/screenSize.h, camera.zoom)
    --TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_GAME)
    --TwRenderSetColorF4(1, 0, 0, 0.5)
    --TwRenderQuad(worldViewRect.x, worldViewRect.y, worldViewRect.w, worldViewRect.h)

    TwRenderSetDrawSpace(2) -- HUD

    -- draw dialog bubbles
    for npcID,npc in pairs(npcs) do
        local npcCore = charCores[npcID]
        if not npcCore then
            return
        end

        local Margin = 10
        local bubbleTextW = 161
        local bubbleW = bubbleTextW
        local fontSize = 13

        -- calculate text size
        local textSize = TwCalculateTextSize({
            text = npc.dialog_line.text,
            font_size = fontSize,
            line_width = bubbleTextW
        })

        local bubbleH = textSize.h + 2 + Margin*2
        bubbleW = bubbleW + Margin*2

        -- get npc pos in hud space
        local npcUiX = (npcCore.pos_x - worldViewRect.x) / worldViewRect.w * uiRect.w
        local npcUiY = (npcCore.pos_y - worldViewRect.y) / worldViewRect.h * uiRect.h

        -- clamp bubble position inside view
        local bubbleX = clamp(npcUiX - bubbleW/2, 0, uiRect.w - bubbleW)
        local bubbleY = clamp(npcUiY - bubbleH - 70, 0, uiRect.h - bubbleH)

        -- draw rect background
        TwRenderSetColorF4(0, 0, 0, 1)
        TwRenderQuad(bubbleX, bubbleY, bubbleW, bubbleH)
        
        -- draw tail
        local tailTopX1 = clamp(npcUiX - 5, 20 - 5, uiRect.w - 20 - 5)
        local tailTopX2 = clamp(npcUiX + 5, 20 + 5, uiRect.w - 20 + 5)
        local tailTopY = bubbleY + bubbleH
        local tailX = npcUiX
        local tailY = npcUiY - 50

        TwRenderSetFreeform({
            tailTopX1, tailTopY,
            tailTopX2, tailTopY,
            tailX, tailY,
            tailX, tailY,
        })
        TwRenderDrawFreeform(0, 0)

        --TwRenderSetColorF4(1, 0, 0, 1)
        --TwRenderQuad(bubbleX+Margin, bubbleY+Margin, textSize.w, textSize.h)
        
        -- draw text
        TwRenderDrawText({
            text = npc.dialog_line.text,
            font_size = fontSize,
            color = {1, 1, 1, 1},
            rect = {bubbleX+Margin, bubbleY+Margin, bubbleTextW}
        })

        local question = npc.question
        if not question then
            -- close enough to npc to interact, display prompt
            if distance(vec2(localCore.pos_x, localCore.pos_y), vec2(npcCore.pos_x,npcCore.pos_y)) < 400 then
                DrawInteractPrompt(clientLocalTime, npcUiX, tailTopY + 10, 30)
            end
        else
            -- get cusror pos in hud space
            local cursorPos = TwGetCursorPosition()
            local cursorUiX = (cursorPos.x - worldViewRect.x) / worldViewRect.w * uiRect.w
            local cursorUiY = (cursorPos.y - worldViewRect.y) / worldViewRect.h * uiRect.h

            local firstRect = {
                x = bubbleX + bubbleW + 5,
                y = bubbleY,
                w = 180,
                h = 30,
            }

            local secondRect = {
                x = bubbleX + bubbleW + 5,
                y = bubbleY + firstRect.h + 5,
                w = firstRect.w,
                h = firstRect.h,
            }

            local fontSize = 12
            local textSize1 = TwCalculateTextSize({
                text = question.answer1,
                font_size = fontSize,
            })
            local textSize2 = TwCalculateTextSize({
                text = question.answer2,
                font_size = fontSize,
            })

            npc.hoveredID = nil
            local color = {0, 0.5, 0, 1}
            if IsPointInsideRect({x = cursorUiX, y = cursorUiY}, firstRect) then
                color = {0, 1.0, 0, 1}
                firstRect.w = firstRect.w + 2
                npc.hoveredID = 0
            end
            DrawRect(firstRect, color)

            TwRenderDrawText({
                text = question.answer1,
                font_size = fontSize,
                color = {1, 1, 1, 1},
                rect = { firstRect.x + 10, firstRect.y + firstRect.h/2 - textSize1.h/2}
            })

            local color = {0.5, 0, 0, 1}
            if IsPointInsideRect({x = cursorUiX, y = cursorUiY}, secondRect) then
                color = {1.0, 0, 0, 1}
                secondRect.w = secondRect.w + 2
                npc.hoveredID = 1
            end
            DrawRect(secondRect, color)

            TwRenderDrawText({
                text = question.answer2,
                font_size = fontSize,
                color = {1, 1, 1, 1},
                rect = { secondRect.x + 10, secondRect.y + secondRect.h/2 - textSize2.h/2}
            })

            TwRenderSetTexture(-1)
            TwRenderSetColorF4(0, 1, 1, 1)
            TwRenderQuadCentered(cursorUiX, cursorUiY, 10, 10)
        end
    end

    -- get npc pos in hud space
    local localCoreUiX = (localCore.pos_x - worldViewRect.x) / worldViewRect.w * uiRect.w
    local localCoreUiY = (localCore.pos_y - worldViewRect.y) / worldViewRect.h * uiRect.h
    DrawItemNotification(localCoreUiX, localCoreUiY, ui.receivedItemTime, clientLocalTime)

    DrawBackpackIcon(game.newItems)

    if ui.backpackIsOpen then
        DrawBackpackInventory()
        DrawMouseCursor()
    end
end

function OnMessage(packet)
    --PrintTable(packet)
    
    if packet.tw_msg_id == Teeworlds.NETMSGTYPE_SV_CLIENTINFO then
        if packet.m_Local == 1 then
            localClientID = packet.m_ClientID + 1 -- get the local ID
            -- TODO = this is inconvenient...
        end
    elseif packet.mod_id == 0x1 then
        local line = TwNetPacketUnpack(packet, {
            "i32_npc_cid",
            "str128_text"
        })

        PrintTable(line)
        if not npcs[line.npc_cid+1] then
            npcs[line.npc_cid+1] = CreateNPC()
        end
        npcs[line.npc_cid+1].dialog_line = line
    elseif packet.mod_id == 0x2 then
        print("received item")
        ui.receivedItemTime = game.localTime
        game.newItems = 1
        game.gotItem = true
    elseif packet.mod_id == 0x3 then
        local question = TwNetPacketUnpack(packet, {
            "i32_npc_cid",
            "str32_answer1",
            "str32_answer2",
        })

        PrintTable(question)
        if not npcs[question.npc_cid+1] then
            npcs[question.npc_cid+1] = CreateNPC()
        end
        npcs[question.npc_cid+1].question = question
    end
end

function GotKeyPress(event, key)
    if event.key ~= key then
        return false
    end

    if event.pressed then
        if keyWasReleased[key] == nil or keyWasReleased[key] == true then
            keyWasReleased[key] = false
            return true
        end
    end
    if event.released then
        keyWasReleased[key] = true
    end

    return false
end

function OnInput(event)
    --PrintTable(event)

    if GotKeyPress(event, 102) then -- f
        TwNetSendPacket({
            net_id = 1,
            force_send_now = 1,
        })
    end
    if GotKeyPress(event, 9) then -- tab
        OpenOrCloseBackpack()
    end
    if GotKeyPress(event, 411) then -- left click
        HandleLeftClick()
    end
end

function HandleLeftClick()
    for npcID,npc in pairs(npcs) do
        if npc.question then
            TwNetSendPacket({
                net_id = 2,
                force_send_now = 1,
                i32_answerID = npc.hoveredID
            })
            npc.question = nil
        end
    end
end