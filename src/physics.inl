//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

static INLINE glm::vec3 get_collision_normal( const glm::vec3& normal, const rigid_body& a, const rigid_body& b )
{
    return std::move( normal * a.mass() * b.mass() );
}
