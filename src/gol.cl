__kernel void update_cell(__global unsigned char *grid,
                          __global unsigned char *new_grid, int rows,
                          int cols) {

  int col = get_global_id(0);
  int row = get_global_id(1);
  int index = row * cols + col;
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      int r = row + i;
      int c = col + j;
      if (r >= 0 && r < rows && c >= 0 && c < cols) {
        count += grid[r * cols + c];
      }
    }
  }
  count -= grid[index];
  if (grid[index]) {
    new_grid[index] = (count == 2 || count == 3);
  } else {
    new_grid[index] = (count == 3);
  }
}
