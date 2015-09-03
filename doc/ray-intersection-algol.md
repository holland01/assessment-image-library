Algorithm [here](http://gamedev.stackexchange.com/a/39903)

The algorithm begins by searching for the distance between the bound's planes on each axis. Not all of the bound's planes are searched, however, but rather the front-most with respect to the ray's direction.

For example, if the ray has a direction with signs (+, -, +), the closest planes found for an AABB would have normals (-, 0, 0), (0, +, 0), and (0, 0, -). These normals would each be respective to the ray direction's coordinates. 

The max and min points of the AABB play an essential role in computing these distances. If the bounds is not an AABB, but rather an OBB, we can take the identity points of the max and min (notably, <1, 1, 1> and <-1, -1, -1> respectively), and transform them relative to the OBB's axes (an arbitrary 3x3 matrix transformation, plus a translation offset).

However, this transform will not necessarily represent the max and minimum points properly with respect to the world's axes. In other words, there is a chance that the minimum point may have values which are farther along one of the axes than the corresponding maximum point.

For example, say that we have the following max, min pair after a transformation:

```
max = [ -2, y_max, z_max ], where y_max > y_min and z_max > z_min
min = [ 2, y_min, z_min ]
```

We can see that while the y and z values satisfy the condition that their corresponding maximum values must be greater than their minimum values, it's clear that this is not the case with the x-component.

In order for this to work properly, we need to be swap coordinates between the two points which offend these preconditions.

A simple resolution for this is to define an arbitrary max function which, given two vectors, will return a new vector containing only the maximum values from the two vectors with respect to their coordinates.

We can do the same for the minimum values as well:

```
let smax( a, b ) be a function which, given two scalars, will return the maximum value between the two.

let smin( a, b ) be a function whcih, given two scalars, will return the minimum value between the two.
```

with these functions defined, we can further define their vector counterparts:

```
vmax( va, vb ):
   return vector3( smax( va_x, vb_x ), smax( va_y, vb_y ), smax( va_z, vb_z ) )

vmin( va, vb ):
   return vector3( smin( va_x, vb_x ), smin( va_y, vb_y ), smin( va_z, vb_z ) )
```

Let's then define an arbitrary transform, `T`, which represents the orientation of the OBB.

Given the identity max and min points, `iMax`, and `iMin`, we can find our partial max and min points `min0`, and `max0`:

```
min0 = T * iMin = T * <-1, -1, -1>
max0 = T * iMax = T * <1, 1, 1>
```

Next, define points `max` and `min` as follows:
```
max = vmax( max0, min0 )
min = vmin( max0, min0 )
```

This ensures that the all values for max are greater than all values for min, which allows for us to effectively evaluate the desired set of planes.

For each coordinate of the ray's position and direction, we perform the following.

Let `ray` be a struct defined as follows:

```
struct ray:
    vec3 position
    vec3 direction
```

We can then define the following routine, given a ray, `R`, whose position and direction are represented relative to the world's origin and orientation:

```
vec3 maxT = <0, 0, 0>

for i in range(0, 3):
    if ( R.direction[ i ] >= 0 ):
		maxT[ i ] = ( min[ i ] - R.position[ i ] ) / R.direction[ i ]
	else:
		maxT[ i ] = ( max[ i ] - R.position[ i ] ) / R.direction[ i ]

```

We make the assertion all elements of maxT are >= 0. This is proved accordingly.

### Proof I.

~~Given two arbitrary points, max and min, which have been constructed and oriented relative to the world's axes~~

Addendum: This proof is dependent on the assumption that the modified max and min points, constructed from the obb transformation and then having their coordinates
thus swapped via vmax and vmin will produce new max and min points...both of which are still of the eight vertices of the OBB. This actually is false, and a new resolution should be constructed for the sake of this (think vector projection).
