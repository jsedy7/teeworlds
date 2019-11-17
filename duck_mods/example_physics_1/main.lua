-- main mod file

function OnLoad()
    print("Example Physics 1");
    TwSetHudPartsShown({
        health = false,
        armor = false,
        ammo = false,
        time = false,
        killfeed = false,
        score = false,
        chat = true,
        scoreboard = true,
    })
end

function OnRender(LocalTime, intraTick)
    TwRenderSetDrawSpace(1)

    local cores = TwPhysGetCores()
    local joints = TwPhysGetJoints()

    TwRenderSetDrawSpace(1)
    TwRenderSetTexture(-1)
    
    for k,core in pairs(cores) do
        TwRenderSetColorF4(1, 1, 1, 1)
        TwRenderDrawCircle(core.x, core.y, core.radius)

        TwRenderSetColorF4(1, 0, 1, 0.5)
        TwRenderQuadCentered(core.x, core.y, core.radius * 2, core.radius * 2)
    end

    for k,joint in pairs(joints) do
        if joint.core1_id ~= nil or joint.core2_id ~= nil then
            local core1 = cores[joint.core1_id]
            local core2 = cores[joint.core2_id]

            TwRenderSetColorF4(0, 0.2, 1, 0.85)
            TwRenderDrawLine(core1.x, core1.y, core2.x, core2.y, 10)
        end
    end
end