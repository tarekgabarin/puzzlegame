#include "platform.h"

// The one shared platform model. Loaded once at startup and reused by every
// platform instance. Kept static so it is private to this translation unit.
static Model platformModel;

// ----------------------------------------------------------------------------
// Cube geometry reference data
//
// A box has 8 corners, indexed so that bit 0 = +X, bit 1 = +Y, bit 2 = +Z:
//
//   0: (-, -, -)   1: (+, -, -)   2: (+, +, -)   3: (-, +, -)
//   4: (-, -, +)   5: (+, -, +)   6: (+, +, +)   7: (-, +, +)
//
// Each face has 4 corners listed in counter-clockwise order when viewed from
// OUTSIDE the cube. CCW = front-facing in OpenGL / Raylib, so getting this
// order wrong makes the face invisible (back-face culling removes it).
// ----------------------------------------------------------------------------
static const int FACE_CORNERS[6][4] = {
    {4, 5, 6, 7}, // front  (+Z)
    {1, 0, 3, 2}, // back   (-Z)
    {5, 1, 2, 6}, // right  (+X)
    {0, 4, 7, 3}, // left   (-X)
    {7, 6, 2, 3}, // top    (+Y)
    {0, 1, 5, 4}, // bottom (-Y)
};

static const float FACE_NORMALS[6][3] = {
    { 0.0f,  0.0f,  1.0f}, // front
    { 0.0f,  0.0f, -1.0f}, // back
    { 1.0f,  0.0f,  0.0f}, // right
    {-1.0f,  0.0f,  0.0f}, // left
    { 0.0f,  1.0f,  0.0f}, // top
    { 0.0f, -1.0f,  0.0f}, // bottom
};

// Appends one axis-aligned box to the mesh arrays.
// The vCursor and tCursor pointers track the next free vertex / triangle slot
// and are advanced by 24 and 12 respectively on return.
//
// Each face gets its own 4 vertices (not shared with other faces) because
// every vertex stores exactly one normal, and adjacent faces have different
// normals. That is why a cube needs 24 vertices, not 8.
static void AppendBox(Mesh *mesh, int *vCursor, int *tCursor,
                      Vector3 center, float w, float h, float d,
                      Color color) {
    float hx = w * 0.5f;
    float hy = h * 0.5f;
    float hz = d * 0.5f;

    // The 8 corners of this box in world space.
    Vector3 corners[8] = {
        { center.x - hx, center.y - hy, center.z - hz }, // 0
        { center.x + hx, center.y - hy, center.z - hz }, // 1
        { center.x + hx, center.y + hy, center.z - hz }, // 2
        { center.x - hx, center.y + hy, center.z - hz }, // 3
        { center.x - hx, center.y - hy, center.z + hz }, // 4
        { center.x + hx, center.y - hy, center.z + hz }, // 5
        { center.x + hx, center.y + hy, center.z + hz }, // 6
        { center.x - hx, center.y + hy, center.z + hz }, // 7
    };

    // Standard quad UVs (0..1) mapped to each face's 4 corners in order.
    const float faceUVs[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    for (int f = 0; f < 6; f++) {
        int baseVertex = *vCursor;

        for (int i = 0; i < 4; i++) {
            Vector3 v = corners[FACE_CORNERS[f][i]];

            mesh->vertices[(*vCursor)*3 + 0] = v.x;
            mesh->vertices[(*vCursor)*3 + 1] = v.y;
            mesh->vertices[(*vCursor)*3 + 2] = v.z;

            mesh->normals[(*vCursor)*3 + 0] = FACE_NORMALS[f][0];
            mesh->normals[(*vCursor)*3 + 1] = FACE_NORMALS[f][1];
            mesh->normals[(*vCursor)*3 + 2] = FACE_NORMALS[f][2];

            mesh->texcoords[(*vCursor)*2 + 0] = faceUVs[i*2 + 0];
            mesh->texcoords[(*vCursor)*2 + 1] = faceUVs[i*2 + 1];

            mesh->colors[(*vCursor)*4 + 0] = color.r;
            mesh->colors[(*vCursor)*4 + 1] = color.g;
            mesh->colors[(*vCursor)*4 + 2] = color.b;
            mesh->colors[(*vCursor)*4 + 3] = color.a;

            (*vCursor)++;
        }

        // Split the quad into 2 triangles, keeping CCW winding:
        //   triangle A = (base+0, base+1, base+2)
        //   triangle B = (base+0, base+2, base+3)
        mesh->indices[(*tCursor)*3 + 0] = baseVertex + 0;
        mesh->indices[(*tCursor)*3 + 1] = baseVertex + 1;
        mesh->indices[(*tCursor)*3 + 2] = baseVertex + 2;
        (*tCursor)++;

        mesh->indices[(*tCursor)*3 + 0] = baseVertex + 0;
        mesh->indices[(*tCursor)*3 + 1] = baseVertex + 2;
        mesh->indices[(*tCursor)*3 + 2] = baseVertex + 3;
        (*tCursor)++;
    }
}

// Builds the complete platform mesh: 1 filled body + 12 thick edge cuboids.
//
// Vertex colors are the trick that lets one mesh have both a tintable body
// and untintable (black) edges. Raylib's default shader computes:
//     finalColor = texelColor * colDiffuse * vertexColor
// colDiffuse includes the tint passed to DrawModel, so:
//     body   = white_texel * green_tint * WHITE_vertex = green
//     edges  = white_texel * green_tint * BLACK_vertex = black   (0 * x = 0)
static Mesh GenPlatformMesh(void) {
    const int BOX_COUNT     = 13;  // 1 body + 12 edges
    const int VERTS_PER_BOX = 24;  // 4 vertices per face * 6 faces
    const int TRIS_PER_BOX  = 12;  // 2 triangles per face * 6 faces

    Mesh mesh = { 0 };
    mesh.vertexCount   = BOX_COUNT * VERTS_PER_BOX;
    mesh.triangleCount = BOX_COUNT * TRIS_PER_BOX;

    mesh.vertices  = (float *)         MemAlloc(mesh.vertexCount   * 3 * sizeof(float));
    mesh.normals   = (float *)         MemAlloc(mesh.vertexCount   * 3 * sizeof(float));
    mesh.texcoords = (float *)         MemAlloc(mesh.vertexCount   * 2 * sizeof(float));
    mesh.colors    = (unsigned char *) MemAlloc(mesh.vertexCount   * 4 * sizeof(unsigned char));
    mesh.indices   = (unsigned short *)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    int vCursor = 0;
    int tCursor = 0;

    const float W = PLATFORM_SIZE;
    const float H = PLATFORM_HEIGHT;
    const float D = PLATFORM_SIZE;
    const float T = BORDER_THICKNESS;

    // 1) Main body in WHITE so the tint color passes through unchanged.
    AppendBox(&mesh, &vCursor, &tCursor,
              (Vector3){ 0.0f, 0.0f, 0.0f },
              W, H, D, WHITE);

    // 2) 12 edge cuboids in BLACK.
    // Each edge is extended by T in its long axis so neighbours fully overlap
    // at the 8 corners and leave no gaps.
    const float ex = W + T;   // X-axis edge length
    const float ey = H + T;   // Y-axis edge length
    const float ez = D + T;   // Z-axis edge length

    const float hX = W * 0.5f;
    const float hY = H * 0.5f;
    const float hZ = D * 0.5f;

    // 4 edges running along the X axis (top/bottom of front/back rims).
    Vector3 xEdges[4] = {
        { 0.0f,  hY,  hZ },
        { 0.0f,  hY, -hZ },
        { 0.0f, -hY,  hZ },
        { 0.0f, -hY, -hZ },
    };
    for (int i = 0; i < 4; i++) {
        AppendBox(&mesh, &vCursor, &tCursor, xEdges[i], ex, T, T, BLACK);
    }

    // 4 vertical edges running along the Y axis.
    Vector3 yEdges[4] = {
        {  hX, 0.0f,  hZ },
        {  hX, 0.0f, -hZ },
        { -hX, 0.0f,  hZ },
        { -hX, 0.0f, -hZ },
    };
    for (int i = 0; i < 4; i++) {
        AppendBox(&mesh, &vCursor, &tCursor, yEdges[i], T, ey, T, BLACK);
    }

    // 4 edges running along the Z axis (left/right of top/bottom rims).
    Vector3 zEdges[4] = {
        {  hX,  hY, 0.0f },
        {  hX, -hY, 0.0f },
        { -hX,  hY, 0.0f },
        { -hX, -hY, 0.0f },
    };
    for (int i = 0; i < 4; i++) {
        AppendBox(&mesh, &vCursor, &tCursor, zEdges[i], T, T, ez, BLACK);
    }

    // Upload the CPU arrays to the GPU. "false" = static mesh (never changes).
    UploadMesh(&mesh, false);
    return mesh;
}

void InitPlatformResources(void) {
    Mesh mesh = GenPlatformMesh();
    platformModel = LoadModelFromMesh(mesh);
}

void UnloadPlatformResources(void) {
    // UnloadModel frees both the CPU arrays and GPU buffers for every mesh
    // it owns, plus any materials.
    UnloadModel(platformModel);
}

// Converts integer grid coordinates (gridX, gridZ) to a Vector3 world position.
//
// Each step of 1 on the grid equals one PLATFORM_SIZE in world units, so grid
// tiles at (0, 0) and (1, 0) are spaced exactly one platform-width apart and
// their cube edges meet with no gap.
//
// Y is fixed at 0 because platforms sit flat on the ground plane.
Vector3 GridToWorld(int gridX, int gridZ) {
    return (Vector3){ gridX * PLATFORM_SIZE, 0.0f, gridZ * PLATFORM_SIZE };
}

void DrawPlatform(Platform *platform) {
    Color tint;
    switch (platform->type) {
        case PLATFORM_NORMAL: tint = GREEN; break;
        case PLATFORM_EXIT:   tint = BLUE;  break;
        default:              tint = GRAY;  break;
    }

    Vector3 world = GridToWorld(platform->gridX, platform->gridZ);

    // One draw call: the mesh already contains the body and all 12 edges.
    // The tint multiplies vertex colors, so WHITE body vertices become `tint`
    // and BLACK edge vertices stay BLACK.
    DrawModel(platformModel, world, 1.0f, tint);
}
