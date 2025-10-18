src_dir = "."
obj_dir = "obj"

AddTool(function(s)
	s.cc.flags:Add("-Wall")
	s.cc.flags:Add("-Wextra")
	s.cc.flags:Add("--std=c++23")
    s.cc.flags:Add("-O3")
	s.cc.includes:Add(src_dir)
	s.cc.includes:Add(src_dir.."/include")

	s.cc.Output = function(s, input)
		input = input:gsub("^"..src_dir.."/", "")
		return PathJoin(obj_dir, PathBase(input))
	end
	s.link.Output = function(s, input)
		return input
	end
end)

s = NewSettings()

src = CollectRecursive(PathJoin(src_dir, "*.cpp"))
obj = Compile(s, src)
bin = Link(s, "bin", obj)

PseudoTarget("compile", bin)
PseudoTarget("c", bin)

AddJob("r", "running '"..bin.."'...", "./"..bin)
AddDependency("r", bin)

DefaultTarget("r");
