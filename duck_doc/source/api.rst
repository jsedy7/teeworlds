
Lua API
==============

The Lua back end is based on `LuaJit 2.0.5 <http://luajit.org/>`_.

==============
Functions
==============

|

print
---------------------------------------------------------------------
.. code-block:: js
   
   function print(arg1)


Prints arg1 to console.

**Parameters**

* **arg1**: any

**Returns**

* None

|

require
---------------------------------------------------------------------
.. code-block:: js
   
   function require(file)


Includes lua script file.

**Parameters**

* **file**: string

**Returns**

* None

|

cos
---------------------------------------------------------------------
.. code-block:: js
   
   function cos(angle)


Returns cosine of angle (radians).

**Parameters**

* **angle**: float

**Returns**

* **cosine**: float

|

sin
---------------------------------------------------------------------
.. code-block:: js
   
   function sin(angle)


Returns sine of angle (radians).

**Parameters**

* **angle**: float

**Returns**

* **sine**: float

|

sqrt
---------------------------------------------------------------------
.. code-block:: js
   
   function sqrt(value)


Returns square root of value.

**Parameters**

* **value**: float

**Returns**

* **result**: float

|

atan2
---------------------------------------------------------------------
.. code-block:: js
   
   function atan2(y, x)


Returns atan2 of y, x.

**Parameters**

* **y**: float
* **x**: float

**Returns**

* **result**: float

|

floor
---------------------------------------------------------------------
.. code-block:: js
   
   function floor(num)


Returns floor of num.

**Parameters**

* **num**: float

**Returns**

* **result**: int

|

TwRenderQuad
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderQuad(x, y, width, height)


Draws a quad.

**Parameters**

* **x**: float
* **y**: float
* **width**: float
* **height**: float

**Returns**

* None

|

TwRenderQuadCentered
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderQuadCentered(x, y, width, height)


Draws a quad centered.

**Parameters**

* **x**: float
* **y**: float
* **width**: float
* **height**: float

**Returns**

* None

|

TwRenderSetColorU32
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetColorU32(color)


| Set render color state as uint32 RGBA.
| Example: ``0x7F0000FF`` is half transparent red.

**Parameters**

* **color**: int

**Returns**

* None

|

TwRenderSetColorF4
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetColorF4(r, g, b, a)


| Set render color state as float RGBA.

**Parameters**

* **r**: float (0.0 - 1.0)
* **g**: float (0.0 - 1.0)
* **b**: float (0.0 - 1.0)
* **a**: float (0.0 - 1.0)

**Returns**

* None

|

TwRenderSetTexture
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetTexture(texture_id)


| Set texture for subsequent draws.
| Example: ``TwRenderSetTexture(TwGetModTexture("duck_butt"));``

**Parameters**

* **texture_id**: int

**Returns**

* None

|

TwRenderSetQuadSubSet
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetQuadSubSet(x1, y1, x2, y2)


| Set quad texture coordinates. ``0, 0, 1, 1`` is default.

**Parameters**

* **x1**: float
* **y1**: float
* **x2**: float
* **y2**: float

**Returns**

* None

|

TwRenderSetQuadRotation
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetQuadRotation(angle)


| Set quad rotation.

**Parameters**

* **angle**: float (radians)

**Returns**

* None

|

TwRenderSetTeeSkin
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetTeeSkin(skin)


| **DEPRECATED**
| Set tee skin for next tee draw call.

**Parameters**

* **skin**

.. code-block:: js

	var skin = {
		textures: [
			texid_body,
			texid_marking,
			texid_decoration,
			texid_hands,
			texid_feet,
			texid_eyes
		],

		colors: [
			color_body,
			color_marking,
			color_decoration,
			color_hands,
			color_feet,
			color_eyes
		]
	};

| texid_*: Use TwGetSkinPartTexture() to get this texture id.
| color_*: ``var color = {r: 1, g: 1, b: 1, a: 1};``

**Returns**

* None

|

TwRenderSetFreeform
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetFreeform(array_vertices)


| Set free form quad (custom vertices).
| Every 8 number produces a quad.
| Example: ``0,0, 1,0, 1,1, 0,1``.

**Parameters**

* **array_vertices**: float[]

**Returns**

* None

|

TwRenderSetDrawSpace
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderSetDrawSpace(draw_space_id)


| Select draw space to draw on.
| Example: ``TwRenderSetDrawSpace(Teeworlds.DRAW_SPACE_HUD)``.

**Parameters**

* **draw_space_id**: int

**Returns**

* None

|

TwRenderDrawTeeBodyAndFeet
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderDrawTeeBodyAndFeet(tee)


| **DEPRECATED**
| Draws a tee without hands.

**Parameters**

* **tee**:

.. code-block:: js

	var tee = {
		size: float,
		angle: float,
		pos_x: float,
		pos_y: float,
		is_walking: bool,
		is_grounded: bool,
		got_air_jump: bool,
		emote: int,
	};

**Returns**

* None

|

TwRenderDrawTeeHand
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderDrawTeeHand(tee)


| **DEPRECATED**
| Draws a tee hand.

**Parameters**

* **tee**:

.. code-block:: js

	var hand = {
		size: float,
		angle_dir: float,
		angle_off: float,
		pos_x: float,
		pos_y: float,
		off_x: float,
		off_y: float,
	};

**Returns**

* None

|

TwRenderDrawFreeform
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderDrawFreeform(x, y)


| Draws the previously defined free form quad at position **x, y**.

**Parameters**

* **x**: float
* **y**: float

**Returns**

* None

|

TwRenderDrawText
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRenderDrawText(text)


| Draw text.
| Example:

.. code-block:: lua

	TwRenderDrawText({
		text = "This a text",
		font_size = 10,
		line_width = -1,
		pos = {10, 20},
		color = {1, 0, 1, 1}, // rgba (0.0 - 1.0)
		clip = {100, 25, 200, 100}, // x y width height
	});

**Parameters**

* **text**:

.. code-block:: lua

	local text = {
		text: string,
		font_size: float,
		line_width: float,
		pos: float[2],
		color: float[4],
		clip: float[4],
	};

**Returns**

* None

|

TwGetBaseTexture
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetBaseTexture(image_id)


| Get vanilla teeworlds texture id.
| Example: ``TwGetBaseTexture(Teeworlds.IMAGE_GAME)``

**Parameters**

* **image_id**: int

**Returns**

* **texture_id**: int

|

TwGetSpriteSubSet
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetSpriteSubSet(sprite_id)


| Get sprite texture coordinates.
| TODO: example

**Parameters**

* **sprite_id**: int

**Returns**

* **subset**:

.. code-block:: lua

	local subset = {
		x1: float,
		y1: float,
		x2: float,
		y2: float,
	};

|

TwGetSpriteScale
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetSpriteScale(sprite_id)


| Get vanilla teeworlds sprite scale.
| TODO: example

**Parameters**

* **sprite_id**: int

**Returns**

* **scale**: {w: float, w: float}

|

TwGetWeaponSpec
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetWeaponSpec(weapon_id)


| **DEPRECATED**
| Get vanilla teeworlds weapon specifications.
| TODO: example

**Parameters**

* **weapon_id**: int

**Returns**

* **TODO**

|

TwGetModTexture
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetModTexture(image_name)


| Get a mod texture based on its name.
| Example: ``TwGetModTexture("duck_burger")``

**Parameters**

* **image_name**: string

**Returns**

* **texture_id**: int

|

TwGetClientSkinInfo
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetClientSkinInfo(client_id)


| Returns the client's skin info

**Parameters**

* **client_id**: int

**Returns**

* **skin**

.. code-block:: lua

	local skin = {
		textures = {
			texid_body: int,
			texid_marking: int,
			texid_decoration: int,
			texid_hands: int,
			texid_feet: int,
			texid_eyes: int
		},

		colors = {
			color_body: {r, g, b ,a},
			color_marking: {r, g, b ,a},
			color_decoration: {r, g, b ,a},
			color_hands: {r, g, b ,a},
			color_feet: {r, g, b ,a},
			color_eyes
		}
	}

|

TwGetClientCharacterCores
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetClientCharacterCores()


| Get interpolated player character cores.

**Parameters**

* None

**Returns**

* **cores**

.. code-block:: js

	var cores = [
		{
			tick: int,
			vel_x: float,
			vel_y: float,
			angle: float,
			direction: int,
			jumped: int,
			hooked_player: int,
			hook_state: int,
			hook_tick: int,
			hook_x: float,
			hook_y: float,
			hook_dx: float,
			hook_dy: float,
			pos_x: float,
			pos_y: float,
		},
		...
	];


|

TwGetStandardSkinInfo
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetStandardSkinInfo()


| **DEPRECATED**
| Get the standard skin info.

**Parameters**

* None

**Returns**

* **skin**

.. code-block:: js

	var skin = {
		textures: [
			texid_body: int,
			texid_marking: int,
			texid_decoration: int,
			texid_hands: int,
			texid_feet: int,
			texid_eyes: int
		],

		colors: [
			color_body: {r, g, b ,a},
			color_marking: {r, g, b ,a},
			color_decoration: {r, g, b ,a},
			color_hands: {r, g, b ,a},
			color_feet: {r, g, b ,a},
			color_eyes
		]
	};


|

TwGetSkinPartTexture
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetSkinPartTexture(part_id, part_name)


| Get a skin part texture. Vanilla and mod skin parts both work here.
| Example: ``TwGetSkinPartTexture(Teeworlds.SKINPART_BODY, "zombie")``

**Parameters**

* **part_id**: int
* **part_name**: string

**Returns**

* **texture_id**: int


|

TwGetCursorPosition
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetCursorPosition()


| Get weapon cursor position in world space.

**Parameters**

* None

**Returns**

* **cursor_pos**: { x: float, y: float }


|

TwGetUiScreenRect
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetUiScreenRect()


| Get UI screen rect. Useful to draw in UI space.

**Parameters**

* None

**Returns**

* **rect**: { x: float, y: float, w: float, h: float }


|

TwGetScreenSize
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetScreenSize()


| Get screen size.

**Parameters**

* None

**Returns**

* **size**: { w: float, h: float }


|

TwGetCamera
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetCamera()


| Get camera position and zoom.

**Parameters**

* None

**Returns**

* **camera**: { x: float, y: float, zoom: float }


|

TwGetUiMousePos
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetUiMousePos()


| Get screen mouse position in UI coordinates.

**Parameters**

* None

**Returns**

* **pos**: { x: float, y: float }


|

TwGetPixelScale
---------------------------------------------------------------------
.. code-block:: js
   
   function TwGetPixelScale()


| Get current draw plane pixel scale. Useful to draw using pixel size.
| Example:

.. code-block:: js

	const pixelScale = TwGetPixelScale();
	var wdith = widthInPixel * pixelScale.x;
	TwRenderQuad(0, 0, width, 50);


**Parameters**

* None

**Returns**

* **scale**: { x: float, y: float }


|

TwPhysGetCores
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPhysGetCores()


| Get predicted physical cores.


**Parameters**

* None

**Returns**

* **cores**:

.. code-block:: lua

	local cores = {
		{
			x: float,
			y: float,
		},
		...
	}


|

TwPhysGetJoints
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPhysGetJoints()


| Get physical joints.
| *Work in progress*


**Parameters**

* None

**Returns**

* **joints**:

.. code-block:: lua

	local joints = {
		{
			core1_id: int or nil,
			core2_id: int or nil,
		},
		...
	}


|

TwPhysSetTileCollisionFlags
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPhysSetTileCollisionFlags(tile_x, tile_y, flags)


| Modify a map tile's collision flags.
| Example: ``TwMapSetTileCollisionFlags(tx, ty, 0); // air``
| See collision.h for flags.
| TODO: give easy access to flags?

**Parameters**

* **tile_x**: int
* **tile_y**: int
* **flags**: int

**Returns**

* None


|

TwDirectionFromAngle
---------------------------------------------------------------------
.. code-block:: js
   
   function TwDirectionFromAngle(angle)


| Get direction vector from angle.

**Parameters**

* **angle**: float

**Returns**

* **dir**: { x: float, y: float }


|

TwSetHudPartsShown
---------------------------------------------------------------------
.. code-block:: js
   
   function TwSetHudPartsShown(hud)


| Show/hide parts of the hud.
| Not specified parts are unchanged.
| Example:

.. code-block:: js

	var hud = {
		health: 0,
		armor: 0,
		ammo: 1,
		time: 0,
		killfeed: 1,
		score: 1,
		chat: 1,
		scoreboard: 1,
		weapon_cursor: 1,
	};

	TwSetHudPartsShown(hud);

**Parameters**

* **hud**

.. code-block:: js

	var hud = {
		health: bool,
		armor: bool,
		ammo: bool,
		time: bool,
		killfeed: bool,
		score: bool,
		chat: bool,
		scoreboard: bool,
		weapon_cursor: bool,
	};

**Returns**

* None


|

TwNetSendPacket
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetSendPacket(packet)


| Send a packet.
| Packet object needs to be formatted to add type information, example:

.. code-block:: lua

	local packet = {
		net_id = 1478,
		force_send_now = 0,

		i32_blockID = 1,
		i32_flags = 5,
		float_pos_x = 180,
		float_pos_y = 20,
		float_vel_x = 0,
		float_vel_y = 0,
		float_width = 1000,
		float_height = 200,
	}


| The first 2 fields are required, the rest are in the form type_name: value.
| Supported types are:

* i32
* u32
* float
* str* (str32_something is a 32 length string)

**Parameters**

* **packet**: user edited object based on:

.. code-block:: lua

	local packet = {
		net_id: int,
		force_send_now: int (0 or 1),
		...
	}

**Returns**

* None


|

TwNetPacketUnpack
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetPacketUnpack(packet, template)


| Unpack packet based on template.
| Each template field will be filled based on the specified type, for example this code:

.. code-block:: lua

	local block = TwNetPacketUnpack(packet, {
		"i32_blockID",
		"i32_flags",
		"float_pos_x",
		"float_pos_y",
		"float_vel_x",
		"float_vel_y",
		"float_width",
		"float_height"
	}

| Will fill the first field (blockID) with the first int in the packet. Same with flags, pos_x will get a float and so on.
| The type is removed on return, so the resulting object looks like this:

.. code-block:: lua

	local block = {
		blockID: int,
		flags: int,
		pos_x: float,
		pos_y: float,
		vel_x: float,
		vel_y: float,
		width: float,
		height:float,
	}

| Supported types are:

* i32
* u32
* float
* str* (str32_something is a 32 length string)

**Parameters**

* **packet**: Packet from OnMessage(packet)
* **template**: user object

**Returns**

* **unpacked**: object


|

TwAddWeapon
---------------------------------------------------------------------
.. code-block:: js
   
   function TwAddWeapon(weapon)


| Simple helper to add a custom weapon.

**Parameters**

* **weapon**

.. code-block:: js

	var weapon = {
		id: int,
		tex_weapon: int, // can be null
		tex_cursor: int, // can be null
		weapon_x: float,
		weapon_y: float,
		hand_x,: float,
		hand_y,: float,
		hand_angle,: float,
		recoil,: float,
	};

**Returns**

* None


|

TwPlaySoundAt
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPlaySoundAt(sound_name, x, y)


| Play a mod sound at position x,y.

**Parameters**

* **sound_name**: string
* **x**: float
* **y**: float

**Returns**

* None


|

TwPlaySoundGlobal
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPlaySoundGlobal(sound_name)


| Play a mod sound globally.

**Parameters**

* **sound_name**: string

**Returns**

* None


|

TwPlayMusic
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPlayMusic(sound_name)


| Play a mod music (will loop).

**Parameters**

* **sound_name**: string

**Returns**

* None


|

TwRandomInt
---------------------------------------------------------------------
.. code-block:: js
   
   function TwRandomInt(min, max)


| Get a random int between min and max, included.
| Example for getting a float instead: ``TwRandomInt(0, 1000000)/1000000.0``

**Parameters**

* **min**: int
* **max**: int

**Returns**

* **rand_int**: int


|

TwCalculateTextSize
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCalculateTextSize(text)


| Calculate text size for the current draw space.
| Example:

.. code-block:: lua

	local size = TwCalculateTextSize({
		text: "Some text",
		font_size: 13,
		line_width: 240
	}

| Note: this is not 100% accurate for now unfortunately...

**Parameters**

* **text**:

.. code-block:: lua

	local text = {
		str: string,
		font_size: float,
		line_width: float
	}

**Returns**

* **size**: { x: float, y: float }


|

TwSetMenuModeActive
---------------------------------------------------------------------
.. code-block:: js
   
   function TwSetMenuModeActive(active)


| Activate or deactivate menu mode.
| All game input is deactivated (tee does not move, shoot or anything else).

**Parameters**

* **active**: bool

**Returns**

* None


|

