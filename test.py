vertices = [(0, 408.447357), (211, 497.289459), (280, 526.342102)]

# Calculate the signed area
def calculate_signed_area(vertices):
    area = 0
    n = len(vertices)
    for i in range(n):
        x1, y1 = vertices[i]
        x2, y2 = vertices[(i + 1) % n]
        area += x1 * y2 - y1 * x2
    return area / 2

# Calculate the centroid
def calculate_centroid(vertices):
    area = calculate_signed_area(vertices)
    cx = 0
    cy = 0
    n = len(vertices)
    for i in range(n):
        x1, y1 = vertices[i]
        x2, y2 = vertices[(i + 1) % n]
        factor = (x1 * y2 - y1 * x2)
        cx += (x1 + x2) * factor
        cy += (y1 + y2) * factor
    if area != 0:
        cx /= (6 * area)
        cy /= (6 * area)
    else:
        cx = cy = 0  # Fallback if area is zero
    return (cx, cy), area

# Execute the calculation
centroid, area = calculate_centroid(vertices)
centroid, area

print("Centroid: ", centroid)
print("Area: ", area)
