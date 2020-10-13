module Layouts
import Statistics

# from 0-1
struct Container
    x :: Cdouble
    y :: Cdouble
    width :: Cdouble
    height :: Cdouble
end

mutable struct Monitor
    m :: Container # monitor area
    w :: Container # window area
    tagset :: Int
    mfact :: Float64
    nmaster :: Int
end

struct cContainerArr
    container :: Ptr{Container}
    size :: Cint
end

function add(box :: Container)
    ccall((:addBox, "juliawm.so"), Cvoid, (Cint, Cint, Cint, Cint), 3, 3, 4, 5)
end

function del()
    ccall((:del, "juliawm.so"), Cvoid, (Cint,), 3)
end

function recurse(arr :: Array{Array{Array{Int}}})
    a = arr[size(arr)] 
end

layoutData = [[Container(0, 0, 1, 1)]]

function arrangeThis(reset :: Bool)
    ccall((:arrangeThis, "./juliawm.so"), Cvoid, (Cint,), reset)
end

# set: which window conf set
# client: current window
function splitContainer(i :: Int, j :: Int, ratio :: Float64)
    global layoutData
    i = min(i, length(layoutData))
    j = min(j, length(layoutData[i]))
    container = layoutData[i][j]

    prevHeight = container.height
    container = Container(container.x, container.y, container.width,
                          container.height * ratio)
    println(prevHeight * (1-ratio))
    newContainer = Container(container.x, container.y + container.height,
                             container.width, prevHeight * (1-ratio))
    layoutData[i][j] = container

    insert!(layoutData, i+1, layoutData[i])
    push!(layoutData[i+1], newContainer)
    arrangeThis(false)
end

# set: which window conf set
# client: current window
function vsplitContainer(i :: Int, j :: Int, ratio :: Float64)
    global layoutData
    i = min(i, length(layoutData))
    j = min(j, length(layoutData[i]))
    container = layoutData[i][j]

    prevWidth = container.width
    container = Container(container.x, container.y, container.width * ratio,
                          container.height)
    newContainer = Container(container.x + container.width, container.y,
                             prevWidth * (1-ratio), container.height)
    layoutData[i][j] = container

    insert!(layoutData, i+1, layoutData[i])
    push!(layoutData[i+1], newContainer)
    arrangeThis(false)
end

function mergeContainer(i :: Int, j1 :: Int, j2 :: Int)
    global layoutData
    println(i)
    i = min(i, length(layoutData))
    j1 = min(j1, length(layoutData[i]))
    j2 = min(j2, length(layoutData[i]))
    container1 = layoutData[i][j1]
    container2 = layoutData[i][j2]

    println("works")
    x = min(container1.x, container2.x)
    y = min(container1.y, container2.y)
    width = max(container1.x + container1.width,
                container2.x + container2.width) - x
    height = max(container1.y + container1.height,
                container2.y + container2.height) - y
    newContainer = Container(x, y, width, height)

    replace!(layoutData[i], j1, newContainer)
    delete!(layoutData[i], j2)
    arrangeThis(false)
end

function tile(n :: Int) :: Ptr{cContainerArr}
    println("tile")
    global layoutData

    layoutData = [
                  [
                   Container(0, 0, 1, 1);
                  ],
                  [
                   Container(0, 0, 1/2, 1),
                   Container(1/2, 0, 1/2, 1),
                  ],
                  [
                   Container(0, 0, 1/2, 1),
                   Container(1/2, 0, 1/2, 1/2),
                   Container(1/2, 1/2, 1/2, 1/2),
                  ],
                  [
                   Container(0, 0, 1/2, 1),
                   Container(1/2, 0, 1/2, 1/3),
                   Container(1/2, 1/3, 1/2, 1/3),
                   Container(1/2, 2/3, 1/2, 1/3),
                  ],
                  [
                   Container(0  , 0  , 1/2, 1  ),
                   Container(1/2, 0  , 1/2, 1/4),
                   Container(1/2, 1/4, 1/2, 1/4),
                   Container(1/2, 2/4, 1/2, 1/4),
                   Container(1/2, 3/4, 1/2, 1/4),
                  ],
                 ]
    return update(n)
end

function monocle(n :: Int) :: Ptr{cContainerArr}
    global layoutData = [[
            Container(1/3, 1/2, 2/3, 1/2)
           ]]

    println("monocle")
    return update(n)
end

# return array of arrangement
function update(n :: Int) :: Ptr{cContainerArr}
    global layoutData
    n = max(1, min(n, length(layoutData)))
    res = cContainerArr(pointer(layoutData[n]), length(layoutData[n]))
    res = pointer([res])
    return res
end
end
