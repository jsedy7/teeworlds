// main mod file

print('Ducktape version = ' + Duktape.version);

function OnUpdate(clientLocalTime)
{
    //print("Hello from the dark side! " + someInt);
    RenderQuad(10.0, 10.0, 100.0 + Math.sin(clientLocalTime) * 50.0, 50.0);
}