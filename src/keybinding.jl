include("config.jl")

function XStringToKeysym(str :: String)
    return ccall((:XStringToKeysym, corePath), Cint, (Cstring,), str)
end

function XKeysymToString(sym) :: String
    res = ccall((:XKeysymToString, corePath), Cstring, (Cint,), sym)
    return unsafe_string(res)
end

function modToString(mod) :: String
    res = ""
    mask = x->2^(x-1)
    for i in 1:8
        if mod & mask(i) != 0
            res *= "$(mods[i]) "
        end
    end
    return res
end

function symToBinding(mod, sym) :: String
    mods = modToString(mod)
    key = XKeysymToString(sym)
    return "$(mods)$(key)"
end

function keyPressed(mod, sym) :: Bool
    bind = symToBinding(mod, sym)
    return processBinding(bind, keys)
end

function buttonPressed(mod, sym) :: Bool
    bind = symToBinding(mod, sym)
    return processBinding(bind, buttons)
end

function isSameKeybind(bind :: String, bind2 :: String) :: Bool
    result = bind2
    for key in split(bind, " ")
        if !isempty(key)
            if !occursin(key, bind2)
                # exit this function if key not in elem
                return false
            else
                result = replace(result, key => "")
            end
        end
    end
    result = strip(result)
    return isempty(result)
end

function processBinding(bind :: String, arr) :: Bool
    println(bind)
    handled = false
    for elem in arr
        i = 0 :: Int
        if isSameKeybind(bind, elem[1])
            elem[2]()
            handled = true
        end
    end
    return handled
end
