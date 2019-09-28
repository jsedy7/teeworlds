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

    TwRenderSetDrawSpace(1 /*DRAW_SPACE_HUD*/);

    ui.dialog_lines.forEach(function(line) {
        
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