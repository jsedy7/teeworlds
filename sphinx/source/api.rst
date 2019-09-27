
Javascript API
==============

The Js back end is based on `Duktape 2.4.0 <https://duktape.org>`_.

==============
Functions
==============

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

.. code-block:: js

	var subset = {
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

* **None**

**Returns**

* **cursor_pos**: { x: float, y: float }


|

TwMapSetTileCollisionFlags
---------------------------------------------------------------------
.. code-block:: js
   
   function TwMapSetTileCollisionFlags(tile_x, tile_y, flags)


| Modify a map tile's collision flags.
| Example: ``TwMapSetTileCollisionFlags(tx, ty, 0); // air``
| See collision.h for flags.
| TODO: give easy access to flags?

**Parameters**

* **tile_x**: int
* **tile_y**: int
* **flags**: int

**Returns**

* **None**


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

TwCollisionSetStaticBlock
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCollisionSetStaticBlock(block_id, block)


| Creates or modify a collision block (rectangle).
| Note: these are supposed to stay the same size and not move much, if at all.

**Parameters**

* **block_id**: int
* **block**

.. code-block:: js

	var block = {
		flags: int, // collision flags
		pos_x, float,
		pos_y, float,
		width, float,
		height, float,
	};

**Returns**

* **None**


|

TwCollisionClearStaticBlock
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCollisionClearStaticBlock(block_id)


| Removes a collision block.

**Parameters**

* **block_id**: int

**Returns**

* **None**


|

TwCollisionSetDynamicDisk
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCollisionSetDynamicDisk(disk_id, disk)


| Creates or modify a dynamic disk.

**Parameters**

* **disk_id**: int
* **disk**

.. code-block:: js

	var disk = {
		flags: int, // unused at the moment
		pos_x, float,
		pos_y, float,
		vel_x, float,
		vel_y, float,
		radius, float,
		hook_force, float,
	};

**Returns**

* **None**


|

TwCollisionClearDynamicDisk
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCollisionClearDynamicDisk(block_id)


| Removes a dynamic disk.

**Parameters**

* **disk_id**: int

**Returns**

* **None**


|

TwCollisionGetPredictedDynamicDisks
---------------------------------------------------------------------
.. code-block:: js
   
   function TwCollisionGetPredictedDynamicDisks()


| Get predicted dynamic disks.

**Parameters**

* **None**

**Returns**

* **disks**

.. code-block:: js

	var disks = [
		{
			id: int,
			flags: int, // unused at the moment
			pos_x, float,
			pos_y, float,
			vel_x, float,
			vel_y, float,
			radius, float,
			hook_force, float,
		},
		...
	];



|

TwSetHudPartsShown
---------------------------------------------------------------------
.. code-block:: js
   
   function TwSetHudPartsShown(hud)


| Show/hide parts of the hud.
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
	};

	TwSetHudPartsShown(hud);

**Parameters**

* **hud**

.. code-block:: js

	var hud = {
		health: int,
		armor: int,
		ammo: int,
		time: int,
		killfeed: int,
		score: int,
		chat: int,
		scoreboard: int,
	};

**Returns**

* **None**


|

TwNetCreatePacket
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetCreatePacket(packet)


| Network, create a custom packet.

**Parameters**

* **packet**

.. code-block:: js

	var packet = {
		netid: int,
		force_send_now: int,
	};

**Returns**

* **None**


|

TwNetPacketAddInt
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetPacketAddInt(var_int)


| Add an int to the current packet.

**Parameters**

* **var_int**: int

**Returns**

* **None**


|

TwNetPacketAddFloat
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetPacketAddFloat(var_float)


| Add a float to the current packet.

**Parameters**

* **var_float**: float

**Returns**

* **None**


|

TwNetPacketAddString
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetPacketAddString(str, size_limit)


| Add a string to the current packet, with a size limit.
| Example: ``TwNetPacketAddString("Dune likes cheese", 32)``

**Parameters**

* **str**: string
* **size_limit**: int

**Returns**

* **None**


|

TwNetSendPacket
---------------------------------------------------------------------
.. code-block:: js
   
   function TwNetSendPacket()


| Send current packet.

**Parameters**

* **None**

**Returns**

* **None**


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

* **None**


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

* **None**


|

TwPlaySoundGlobal
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPlaySoundGlobal(sound_name)


| Play a mod sound globally.

**Parameters**

* **sound_name**: string

**Returns**

* **None**


|

TwPlayMusic
---------------------------------------------------------------------
.. code-block:: js
   
   function TwPlayMusic(sound_name)


| Play a mod music (will loop).

**Parameters**

* **sound_name**: string

**Returns**

* **None**


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

