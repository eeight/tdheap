#include <stdio.h>
#include <malloc.h>

void grow_array(int **array, int *array_size) {
    *array_size *= 2;

    *array = realloc(*array, (*array_size)*sizeof(int));
}

int main(void) {
    int *array = malloc(sizeof(int));
    int n = 0, array_size = 1;
    int next;
    int i;

    while (scanf("%d", &next) == 1) {
        while (array_size <= n) {
            grow_array(&array, &array_size);
        }
        array[n] = next;
        ++n;
    }

    for (i = 0; i != n; ++i) {
        printf("%d\n", array[i]);
    }

    free(array);
}
