
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

* **skin**:

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

* **skin**:

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

