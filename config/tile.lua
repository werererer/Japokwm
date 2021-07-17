require "tileutils"

local X<const> = 1
local Y<const> = 2
local WIDTH<const> = 3
local HEIGHT<const> = 4

function Is_equally_affected_by_resize_of(container, container2, d)
    local resize = false
    if d == info.direction.top then
        resize = Is_approx_equal(container2[Y], container[Y])
    elseif d == info.direction.bottom then
        resize = Is_approx_equal(container2[Y] + container2[HEIGHT], container[Y] + container[HEIGHT])
    elseif d == info.direction.left then
        resize = Is_approx_equal(container2[X], container[X])
    elseif d == info.direction.right then
        resize = Is_approx_equal(container2[X] + container2[WIDTH], container[X] + container[WIDTH])
    end
    return resize
end

-- finds containers that are affected by the container at i,j
function Get_resize_affected_containers(lt_data_el, o_lt_data_el, i, d, get_container_func, is_effected_by_func)
    local container = lt_data_el[i]
    local list = {}

    for j = 1, #lt_data_el do
        local con = lt_data_el[j]
        local alt_con = get_container_func(container, d)

        if i ~= j then
            if is_effected_by_func(o_lt_data_el[i], o_lt_data_el[j], d) then
                -- convert relative to absolute box
                local ret_con = {con[X], con[Y], con[WIDTH], con[HEIGHT], j}
                ret_con[X] = (ret_con[X]-alt_con[X])/alt_con[WIDTH]
                ret_con[Y] = (ret_con[Y]-alt_con[Y])/alt_con[HEIGHT]
                ret_con[WIDTH] = ret_con[WIDTH]/alt_con[WIDTH]
                ret_con[HEIGHT] = ret_con[HEIGHT]/alt_con[HEIGHT]
                table.insert(list, ret_con)
            end
        end
    end
    return list
end

function Move_this_container(n, d)
    -- local i = math.max(math.min(action.get_this_container_count(), #Layout_data), 1)
    -- local j = math.min(info.this_container_position(), #Layout_data[i])
    -- local container = Layout_data[i][j]
    -- Layout_data[i][j] = Move_container(container, n, d)
    -- action.arrange()
end

function Resize_this_container(n, d)
    -- local i = math.max(math.min(action.get_this_container_count(), #Layout_data), 1)
    -- local j = math.min(action.client_pos(), #Layout_data[i])
    -- Layout_data[i][j] = Resize_container(Layout_data[i][j], n, d)
    -- action.arrange()
end

--
-- returns whether container2 is affected
function Is_affected_by_resize_of(container, container2, d)
    local resize = false

    if d == info.direction.top then
        local right = Is_container_right_to(container, container2)
        local left = Is_container_left_to(container, container2)
        local container_is_higher = Is_container_over(container, container2)

        resize = container_is_higher and not (left or right)
    elseif d == info.direction.bottom then
        local right = Is_container_right_to(container, container2)
        local left = Is_container_left_to(container, container2)
        local container_is_lower = Is_container_under(container, container2)

        resize = container_is_lower and not (left or right)
    elseif d == info.direction.left then
        local over = Is_container_over(container, container2)
        local under = Is_container_under(container, container2)
        local container_is_left = Is_container_left_to(container, container2)

        resize = container_is_left and not (over or under)
    elseif d == info.direction.right then
        local over = Is_container_over(container, container2)
        local under = Is_container_under(container, container2)
        local container_is_right = Is_container_right_to(container, container2)

        resize = container_is_right and not (over or under)
    end

    return resize
end

local function apply_resize_function(lt_data_el, o_lt_data_el, i, n, directions)
    local main_con = lt_data_el[i]
    for x = 1,#directions do
        local dir = directions[x]
        local resize_main_containers = Get_resize_affected_containers(lt_data_el, o_lt_data_el, i, dir, Get_main_container, Is_equally_affected_by_resize_of)
        local resize_containers = Get_resize_affected_containers(lt_data_el, o_lt_data_el, i, dir, Get_alternative_container, Is_affected_by_resize_of)
        main_con = Deep_copy(Move_resize(main_con, 0, n, dir))
        local alt_con = Get_alternative_container(main_con, dir)

        for k = 1,#resize_containers do
            local lj = resize_containers[k][5]

            lt_data_el[lj][X] = alt_con[X] + (resize_containers[k][X] * alt_con[WIDTH])
            lt_data_el[lj][Y] = alt_con[Y] + (resize_containers[k][Y] * alt_con[HEIGHT])
            lt_data_el[lj][WIDTH] = resize_containers[k][WIDTH] * alt_con[WIDTH]
            lt_data_el[lj][HEIGHT] = resize_containers[k][HEIGHT] * alt_con[HEIGHT]
        end

        lt_data_el[i] = Deep_copy(main_con)
        for k = 1,#resize_main_containers do
            local lj = resize_main_containers[k][5]
            lt_data_el[lj] = Move_resize(lt_data_el[lj], 0, n, dir)
        end
    end
end

-- TODO refactor and simplify
function Resize_all(lt_data_el, o_layout_data_el, i, n, d)
    if i > #lt_data_el then
        return lt_data_el
    end

    local directions = Get_directions(d)
    local layout_data_element = Deep_copy(lt_data_el)

    if Is_resize_locked(layout_data_element, o_layout_data_el, i, n, directions) then
        return layout_data_element
    end

    apply_resize_function(layout_data_element, o_layout_data_el, i, n, directions)

    return layout_data_element
end

function Set(list)
    local set = {}
    for _,l in ipairs(list) do
        set[l] = true
    end
    return set
end

local function get_layout_data_element_id(o_layout_data)
    return math.max(math.min(info.get_this_container_count(), #o_layout_data), 1)
end

-- returns 0 if not found
local function get_layout_element(layout_data_element_id, resize_data)
    for j=1,#resize_data do
        for h=1, #resize_data[j] do
            if layout_data_element_id == resize_data[j][h] then
                return j
            end
        end
    end
    return 0
end

function Resize_main_all(layout_data, o_layout_data, resize_data, n, direction)
    local layout_data_element_id = get_layout_data_element_id(o_layout_data)
    local layout_id = get_layout_element(layout_data_element_id, resize_data)
    if layout_id == 0 then
        return layout_data
    end

    local resize_element = resize_data[layout_id]
    for i=1,#resize_element do
        local id = resize_element[i]
        if id < #o_layout_data then
            layout_data[id] = Resize_all(layout_data[id], o_layout_data[id], 1, n, direction)
        end
    end
    return layout_data
end
