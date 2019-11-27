-- main mod file

function OnLoad()
    print("Example Render 1");
end

function OnRender(LocalTime, intraTick)
end

function OnRenderPlayer(AnimState, Pos, ClientID)
    --print(AnimState)
    local body = AnimState.body
    body.x = body.x + Pos.x
    body.y = body.y + Pos.y
    local texBody = TwGetSkinPartTexture(0, "kitty")[1]

    TwRenderSetTexture(texBody)

    -- back
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadSubSet(0, 0, 0.5, 0.5)
    TwRenderQuadCentered(body.x, body.y, 64, 64)

    -- body
    TwRenderSetColorF4(1, 0, 0, 1)
    TwRenderSetQuadSubSet(0.5, 0, 1, 0.5)
    TwRenderQuadCentered(body.x, body.y, 64, 64)

    -- shadow
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadSubSet(0, 0.5, 0.5, 1)
    TwRenderQuadCentered(body.x, body.y, 64, 64)

    -- outline
    TwRenderSetColorF4(1, 1, 1, 1)
    TwRenderSetQuadSubSet(0.5, 0.5, 1, 1)
    TwRenderQuadCentered(body.x, body.y, 64, 64)
    return true
end