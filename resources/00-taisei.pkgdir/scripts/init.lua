
print('Hello, Lua!')

function ls(path)
	iter, err = vfs.iterdir(path)
	if not iter then error(err) end

	for p in iter do
		print(p)
	end
end

function cat(path)
	file, err = vfs.open(path)
	if not file then error(err) end
	print(file:read())
end
