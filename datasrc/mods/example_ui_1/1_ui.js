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