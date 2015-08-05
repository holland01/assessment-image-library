//-------------------------------------------------------------------------------------------------------
// util
//-------------------------------------------------------------------------------------------------------

static INLINE glm::vec3 P_GenericCollideNormal( const glm::vec3& normal, const body_t& a, const body_t& b )
{
    return std::move( normal * a.GetMass() * b.GetMass() );
}
