// UI stuff

function CalcScreenParams(Amount, WMax, HMax, Aspect)
{
    var f = Math.sqrt(Amount) / Math.sqrt(Aspect);
	var w = f*Aspect;
	var h = f;

	// limit the view
	if(w > WMax)
	{
		w = WMax;
		h = w/Aspect;
	}

	if(h > HMax)
	{
		h = HMax;
		w = h*Aspect;
    }
    
    return [w, h];
}

function GetWorldViewRect(CenterX, CenterY, Aspect, Zoom)
{
    var params = CalcScreenParams(1150*1000, 1500, 1050, Aspect);
    var Width = params[0];
    var Height = params[1];
    Width *= Zoom;
    Height *= Zoom;
    
    return {
        x: CenterX-Width/2,
        y: CenterY-Height/2,
        w: Width,
        h: Height,
    };
}

function IsPointInsideRect(point, rect)
{
    return (point.x >= rect.x && point.x < rect.x + rect.w && point.y >= rect.y && point.y < rect.y + rect.h);
}

function DrawRect(rect, rectColor)
{
    TwRenderSetTexture(-1);
    TwRenderSetColorF4(rectColor[0], rectColor[1], rectColor[2], rectColor[3]);
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h);
}

function DrawBorderRect(rect, borderWith, rectColor, borderColor)
{
    const pixelScale = TwGetPixelScale();
	const borderW = borderWith * pixelScale.x;
    const borderH = borderWith * pixelScale.y;
    
    TwRenderSetTexture(-1);

    TwRenderSetColorF4(rectColor[0], rectColor[1], rectColor[2], rectColor[3]);
    TwRenderQuad(rect.x, rect.y, rect.w, rect.h);

    TwRenderSetColorF4(borderColor[0], borderColor[1], borderColor[2], borderColor[3]);
    TwRenderQuad(rect.x, rect.y, rect.w, borderH);
    TwRenderQuad(rect.x, rect.y + rect.h - borderH, rect.w, borderH);
    TwRenderQuad(rect.x, rect.y, borderW, rect.h);
    TwRenderQuad(rect.x + rect.w - borderW, rect.y, borderW, rect.h);
}