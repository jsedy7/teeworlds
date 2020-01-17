Lua = {
	basepath = PathDir(ModuleFilename()),

	OptFind = function (name, required)
		local check = function(option, settings)
			option.value = false
			option.use_pkgconfig = false
			option.use_winlib = 0
			option.use_linuxlib = false
			option.lib_path = nil

			if ExecuteSilent("pkg-config luajit") == 0 then
				if ExecuteSilent("pkg-config --modversion luajit | grep 2.0.5") == 0 then
					option.value = true
					option.use_pkgconfig = true
				-- let it be, we have a fallback
				-- else
				-- 	print("luajit: wrong version:")
				-- 	Execute("pkg-config --modversion luajit")
				end
			end

			if platform == "linux" and option.use_pkgconfig == false then
				if ExecuteSilent("test -d " .. Lua.basepath .. "/src") ~= 0 then
					print("Unpacking luajit...")
					Execute("cd " .. Lua.basepath .. "&& tar xzf LuaJIT-2.0.5.tar.gz --strip 1")
					print("Building luajit...")
					Execute("cd " .. Lua.basepath .. "&& make")
				end
				option.value = true
				option.use_linuxlib = true
			end

			if platform == "win32" then
				option.value = true
				option.use_winlib = 32
			elseif platform == "win64" then
				option.value = true
				option.use_winlib = 64
			end
		end

		local apply = function(option, settings)
			if(option.use_pkgconfig == nil) then
				check(option, settings)
			end
			if option.use_pkgconfig == true then
				settings.cc.flags:Add("`pkg-config --cflags luajit`")
				settings.link.flags:Add("`pkg-config --libs luajit`")
			elseif option.use_winlib > 0 then
				settings.cc.includes:Add(Lua.basepath .. "/include")
				if option.use_winlib == 32 then
					settings.link.libpath:Add(Lua.basepath .. "/windows/lib32")
				else
					settings.link.libpath:Add(Lua.basepath .. "/windows/lib64")
				end
				settings.link.libs:Add("lua51")
			elseif option.use_linuxlib == true then
				settings.cc.includes:Add(Lua.basepath .. "/include")
				settings.link.libpath:Add(Lua.basepath .. "/src")
				settings.link.libs:Add("luajit-5.1")
			end
		end

		local save = function(option, output)
			output:option(option, "value")
			output:option(option, "use_pkgconfig")
			output:option(option, "use_winlib")
			output:option(option, "use_linuxlib")
		end

		local display = function(option)
			if option.value == true then
				if option.use_pkgconfig == true then return "using pkg-config" end
				if option.use_winlib == 32 then return "using supplied win32 libraries" end
				if option.use_winlib == 64 then return "using supplied win64 libraries" end
				if option.use_linuxlib == true then return "using supplied linux libraries" end
				return "using unknown method"
			else
				if option.required then
					return "not found (required)"
				else
					return "not found (optional)"
				end
			end
		end

		local o = MakeOption(name, 0, check, save, display)
		o.Apply = apply
		o.include_path = nil
		o.lib_path = nil
		o.required = required
		return o
	end
}
