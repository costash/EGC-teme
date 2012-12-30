#include "Mesh.h"


////////////////////////////////////////////////////////////
// OFF FILE READING CODE
////////////////////////////////////////////////////////////

Mesh *
ReadOffFile(const char *filename)
{
  // Open file
  FILE *fp;
  if (!(fp = fopen(filename, "r"))) {
    fprintf(stderr, "Unable to open file %s\n", filename);
    return 0;
  }

  // Allocate mesh structure
  Mesh *mesh = new Mesh();
  if (!mesh) {
    fprintf(stderr, "Unable to allocate memory for file %s\n", filename);
    fclose(fp);
    return 0;
  }

  // Read file
  int nverts = 0;
  int nfaces = 0;
  int nedges = 0;
  int line_count = 0;
  char buffer[1024];
  while (fgets(buffer, 1023, fp)) {
    // Increment line counter
    line_count++;

    // Skip white space
    char *bufferp = buffer;
    while (isspace(*bufferp)) bufferp++;

    // Skip blank lines and comments
    if (*bufferp == '#') continue;
    if (*bufferp == '\0') continue;

    // Check section
    if (nverts == 0) {
      // Read header 
      if (!strstr(bufferp, "OFF")) {
        // Read mesh counts
        if ((sscanf(bufferp, "%d%d%d", &nverts, &nfaces, &nedges) != 3) || (nverts == 0)) {
          fprintf(stderr, "Syntax error reading header on line %d in file %s\n", line_count, filename);
          fclose(fp);
          return NULL;
        }

        // Allocate memory for mesh
        mesh->verts = new Vertex [nverts];
        assert(mesh->verts);
        mesh->faces = new Face [nfaces];
        assert(mesh->faces);
      }
    }
    else if (mesh->nverts < nverts) {
      // Read vertex coordinates
      Vertex& vert = mesh->verts[mesh->nverts++];
      if (sscanf(bufferp, "%f%f%f", &(vert.x), &(vert.y), &(vert.z)) != 3) {
        fprintf(stderr, "Syntax error with vertex coordinates on line %d in file %s\n", line_count, filename);
        fclose(fp);
        return NULL;
      }
    }
    else if (mesh->nfaces < nfaces) {
      // Get next face
      Face& face = mesh->faces[mesh->nfaces++];

      // Read number of vertices in face 
      bufferp = strtok(bufferp, " \t");
      if (bufferp) face.nverts = atoi(bufferp);
      else {
        fprintf(stderr, "Syntax error with face on line %d in file %s\n", line_count, filename);
        fclose(fp);
        return NULL;
      }

      // Allocate memory for face vertices
      face.verts = new Vertex *[face.nverts];
      assert(face.verts);

      // Read vertex indices for face
      for (int i = 0; i < face.nverts; i++) {
        bufferp = strtok(NULL, " \t");
        if (bufferp) face.verts[i] = &(mesh->verts[atoi(bufferp)]);
        else {
          fprintf(stderr, "Syntax error with face on line %d in file %s\n", line_count, filename);
          fclose(fp);
          return NULL;
        }
      }

      // Compute normal for face
      face.normal[0] = face.normal[1] = face.normal[2] = 0;
      Vertex *v1 = face.verts[face.nverts-1];
      for (int i = 0; i < face.nverts; i++) {
        Vertex *v2 = face.verts[i];
        face.normal[0] += (v1->y - v2->y) * (v1->z + v2->z);
        face.normal[1] += (v1->z - v2->z) * (v1->x + v2->x);
        face.normal[2] += (v1->x - v2->x) * (v1->y + v2->y);
        v1 = v2;
      }

      // Normalize normal
      float squared_normal_length = 0.0;
      squared_normal_length += face.normal[0]*face.normal[0];
      squared_normal_length += face.normal[1]*face.normal[1];
      squared_normal_length += face.normal[2]*face.normal[2];
      float normal_length = sqrt(squared_normal_length);
      if (normal_length > 1.0E-6) {
        face.normal[0] /= normal_length;
        face.normal[1] /= normal_length;
        face.normal[2] /= normal_length;
      }
    }
    else {
      // Should never get here
      fprintf(stderr, "Found extra text starting at line %d in file %s\n", line_count, filename);
      break;
    }
  }

  // Check whether read all faces
  if (nfaces != mesh->nfaces) {
    fprintf(stderr, "Expected %d faces, but read only %d faces in file %s\n", nfaces, mesh->nfaces, filename);
  }

  // Close file
  fclose(fp);

	Vector3D center = computeMeshCenter(mesh);
	centrateMesh(mesh, center);

  // Return mesh 
  return mesh;
}



////////////////////////////////////////////////////////////
// PLY FILE READING CODE
////////////////////////////////////////////////////////////

void
PrintStats(Mesh *mesh)
{
  // Compute bounding box
  float xmin = 1.0E30f, ymin = 1.0E30f, zmin = 1.0E30f;
  float xmax = -1.0E30f, ymax = -1.0E30f, zmax = -1.0E30f;
  for (int i = 0; i < mesh->nverts; i++) {
    Vertex& v = mesh->verts[i];
    if (v.x < xmin) xmin = v.x;
    if (v.y < ymin) ymin = v.y;
    if (v.z < zmin) zmin = v.z;
    if (v.x > xmax) xmax = v.x;
    if (v.y > ymax) ymax = v.y;
    if (v.z > zmax) zmax = v.z;
  }

  // Coont triangles, quads, others
  int ntris = 0, nquads = 0, ngons = 0;
  for (int i = 0; i < mesh->nfaces; i++) {
    Face& f = mesh->faces[i];
    if (f.nverts == 3) ntris++;
    else if (f.nverts == 4) nquads++;
    else ngons++;
  }

  // Print mesh stats
  printf("# Vertices = %d\n", mesh->nverts);
  printf("# Faces = %d ( %d %d %d )\n", mesh->nfaces, ntris, nquads, ngons);
  printf("Bounding box = %g %g   %g %g   %g %g\n", xmin, xmax, ymin, ymax, zmin, zmax);
}

// Compute Mesh Center
Vector3D computeMeshCenter(Mesh *mesh)
{
	Vector3D center;
	// Compute bounding box
	float xmin = 1.0E30f, ymin = 1.0E30f, zmin = 1.0E30f;
	float xmax = -1.0E30f, ymax = -1.0E30f, zmax = -1.0E30f;
	for (int i = 0; i < mesh->nverts; ++i)
	{
		Vertex& v = mesh->verts[i];
		if (v.x < xmin) xmin = v.x;
		if (v.y < ymin) ymin = v.y;
		if (v.z < zmin) zmin = v.z;
		if (v.x > xmax) xmax = v.x;
		if (v.y > ymax) ymax = v.y;
		if (v.z > zmax) zmax = v.z;
	}
	center.x = (xmax - xmin) / 2;
	center.y = (ymax - ymin) / 2;
	center.z = (zmax - zmin) / 2;
	return center;
}

// Centrate Mesh to (0, 0, 0)
void centrateMesh(Mesh *mesh, Vector3D center)
{
	for (int i = 0; i < mesh->nverts; ++i)
	{
		mesh->verts[i].x -= center.x;
		mesh->verts[i].y -= center.y;
		mesh->verts[i].z -= center.z;
	}
}


////////////////////////////////////////////////////////////
// PROGRAM ARGUMENT PARSING
////////////////////////////////////////////////////////////

int 
ParseArgs(int argc, char **argv)
{
  // Innocent until proven guilty
  int print_usage = 0;

  // Parse arguments
  argc--; argv++;
  while (argc > 0) {
    if ((*argv)[0] == '-') {
      if (!strcmp(*argv, "-help")) { print_usage = 1; }
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
    else {
      if (!filename) filename = *argv;
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
  }

  // Check filename
  if (!filename || print_usage) {
    printf("Usage: offstats <filename>\n");
    return 0;
  }

  // Return OK status 
  return 1;
}



////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////

//int 
//main(int argc, char **argv)
//{
//  // Parse program arguments
//  if (!ParseArgs(argc, argv)) exit(1);
//
//  // Read mesh
//  mesh = ReadOffFile(filename);
//  if (!mesh) exit(-1);
//
//  // Print statistics
//  PrintStats(mesh);
//
//  // Return success 
//  return 0;
//}