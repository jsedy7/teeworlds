// main mod file

var game = {
};

var ui = {
    show_inventory: false,
    dialog_lines: []
};

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
}

function OnUpdate(clientLocalTime, intraTick)
{
   
}

function OnRender(clientLocalTime, intraTick)
{
    if(ui.show_inventory) {
        DrawInventory();
    }

    ui.dialog_lines.forEach(function(line) {
        TwRenderSetDrawSpace(1 /*DRAW_SPACE_HUD*/);
        TwRenderDrawText({
            str: line.text,
            font_size: 10,
            colors: [1, 1, 1, 1],
            rect: [0, 0, 400, 250]
        });
    });
}

function OnMessage(packet)
{
    if(packet.mod_id == 0x1) {
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