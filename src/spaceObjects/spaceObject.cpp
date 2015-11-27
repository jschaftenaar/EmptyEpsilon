#include "spaceObject.h"
#include "factionInfo.h"
#include "gameGlobalInfo.h"

#include "scriptInterface.h"
/// The SpaceObject is the base for every object which can be seen in space.
/// General properties can read and set for each object. Each object has a position, rotation and collision shape.
REGISTER_SCRIPT_CLASS_NO_CREATE(SpaceObject)
{
    /// Set the position of this object in 2D space, in meters
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, setPosition);
    /// Sets the absolute rotation of this object. In degrees.
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, setRotation);
    /// Gets the position of this object, returns x, y
    /// Example: local x, y = obj:getPosition()
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, getPosition);
    /// Gets the rotation of this object. In degrees.
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, getRotation);
    /// Gets the velocity of the object, in 2D space, in meters/second
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, getVelocity);
    /// Gets the rotational velocity of the object, in degree/second
    REGISTER_SCRIPT_CLASS_FUNCTION(Collisionable, getAngularVelocity);
    
    /// Sets the faction to which this object belongs. Requires a string as input.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, setFaction);
    /// Gets the faction name to which this object belongs.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getFaction);
    /// Sets the faction to which this object belongs. Requires a index in the faction list.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, setFactionId);
    /// Gets the index in the faction list from this object.
    /// Can be used in combination with setFactionId to make sure two objects have the same faction.
    /// Example: other:setFactionId(obj:getFactionId())
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getFactionId);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, setCommsScript);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, isEnemy);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, isFriendly);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getCallSign);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, areEnemiesInRange);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getObjectsInRange);
    /// Return the current amount of reputation points.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getReputationPoints);
    /// Take a certain amount of reputation points, returns true when there are enough points to take. Returns false when there are not enough points and does not lower the points.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, takeReputationPoints);
    /// Add a certain amount of reputation points.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, addReputationPoints);
    /// Get the name of the sector this object is in (A4 for example)
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getSectorName);
    /// Hail a player ship from this object. The ship will get a notification and can accept or deny the hail.
    /// Warning/ToFix: If the player refuses the hail, no feedback is given to the script in any way.
    /// Return true when the hail is enabled with succes. Returns false when the target player cannot be hailed right now (because it's already communicating with something else)
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, openCommsTo);
    /// Hail a player ship from this object. The ship will get a notification and can accept or deny the hail.
    /// Warning/ToFix: If the player refuses the hail, no feedback is given to the script in any way.
    /// Return true when the hail is enabled with succes. Returns false when the target player cannot be hailed right now (because it's already communicating with something else)
    /// This function will display the message given as parameter when the hail is answered.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, sendCommsMessage);
    /// Let this object take damage, the DamageInfo parameter can be empty, or a string which indicates if it's energy, kinetic or emp damage.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, takeDamage);
    /// Set the description of this object, description is visible at the science station.
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, setDescription);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceObject, getDescription);
}

PVector<SpaceObject> space_object_list;

SpaceObject::SpaceObject(float collision_range, string multiplayer_name, float multiplayer_significant_range)
: Collisionable(collision_range), MultiplayerObject(multiplayer_name)
{
    object_radius = collision_range;
    space_object_list.push_back(this);
    faction_id = 0;

    registerMemberReplication(&faction_id);
    registerCollisionableReplication(multiplayer_significant_range);
}

#if FEATURE_3D_RENDERING
void SpaceObject::draw3D()
{
    model_info.render(getPosition(), getRotation());
}
#endif//FEATURE_3D_RENDERING

void SpaceObject::drawOnRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, bool longRange)
{
}

void SpaceObject::drawOnGMRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, bool longRange)
{
}

void SpaceObject::destroy()
{
    onDestroyed();
    MultiplayerObject::destroy();
}

bool SpaceObject::isEnemy(P<SpaceObject> obj)
{
    return factionInfo[faction_id]->states[obj->faction_id] == FVF_Enemy;
}

bool SpaceObject::isFriendly(P<SpaceObject> obj)
{
    return factionInfo[faction_id]->states[obj->faction_id] == FVF_Friendly;
}

void SpaceObject::damageArea(sf::Vector2f position, float blast_range, float min_damage, float max_damage, DamageInfo info, float min_range)
{
    PVector<Collisionable> hitList = CollisionManager::queryArea(position - sf::Vector2f(blast_range, blast_range), position + sf::Vector2f(blast_range, blast_range));
    foreach(Collisionable, c, hitList)
    {
        P<SpaceObject> obj = c;
        if (obj)
        {
            float dist = sf::length(position - obj->getPosition()) - obj->getRadius() - min_range;
            if (dist < 0) dist = 0;
            if (dist < blast_range - min_range)
            {
                obj->takeDamage(max_damage - (max_damage - min_damage) * dist / (blast_range - min_range), info);
            }
        }
    }
}

bool SpaceObject::areEnemiesInRange(float range)
{
    PVector<Collisionable> hitList = CollisionManager::queryArea(getPosition() - sf::Vector2f(range, range), getPosition() + sf::Vector2f(range, range));
    foreach(Collisionable, c, hitList)
    {
        P<SpaceObject> obj = c;
        if (obj && isEnemy(obj))
        {
            if (getPosition() - obj->getPosition() < range + obj->getRadius())
                return true;
        }
    }
    return false;
}

PVector<SpaceObject> SpaceObject::getObjectsInRange(float range)
{
    PVector<SpaceObject> ret;
    PVector<Collisionable> hitList = CollisionManager::queryArea(getPosition() - sf::Vector2f(range, range), getPosition() + sf::Vector2f(range, range));
    foreach(Collisionable, c, hitList)
    {
        P<SpaceObject> obj = c;
        if (obj && getPosition() - obj->getPosition() < range + obj->getRadius())
        {
            ret.push_back(obj);
        }
    }
    return ret;
}

int SpaceObject::getReputationPoints()
{
    if (gameGlobalInfo->reputation_points.size() < faction_id)
        return 0;
    return gameGlobalInfo->reputation_points[faction_id];
}

bool SpaceObject::takeReputationPoints(float amount)
{
    if (gameGlobalInfo->reputation_points.size() < faction_id)
        return false;
     if (gameGlobalInfo->reputation_points[faction_id] < amount)
        return false;
    gameGlobalInfo->reputation_points[faction_id] -= amount;
    return true;
}

void SpaceObject::removeReputationPoints(float amount)
{
    addReputationPoints(-amount);
}

void SpaceObject::addReputationPoints(float amount)
{
    if (gameGlobalInfo->reputation_points.size() < faction_id)
        return;
    gameGlobalInfo->reputation_points[faction_id] += amount;
    if (gameGlobalInfo->reputation_points[faction_id] < 0.0)
        gameGlobalInfo->reputation_points[faction_id] = 0.0;
}

string SpaceObject::getSectorName()
{
    return ::getSectorName(getPosition());
}

bool SpaceObject::openCommsTo(P<PlayerSpaceship> target)
{
    return sendCommsMessage(target, "");
}

bool SpaceObject::sendCommsMessage(P<PlayerSpaceship> target, string message)
{
    if (!target)
        return false;
    switch(target->comms_state)
    {
    case CS_OpeningChannel:
    case CS_BeingHailed:
        if (target->comms_target != this)
            return false;
        break;
    case CS_BeingHailedByGM:
    case CS_ChannelOpen:
    case CS_ChannelOpenPlayer:
    case CS_ChannelOpenGM:
        return false;
    case CS_Inactive:
    case CS_ChannelFailed:
    case CS_ChannelBroken:
        break;
    }
    target->comms_target = this;
    target->comms_target_name = getCallSign();
    target->comms_state = CS_BeingHailed;
    target->comms_incomming_message = message;
    return true;
}

template<> void convert<DamageInfo>::param(lua_State* L, int& idx, DamageInfo& di)
{
    if (!lua_isstring(L, idx))
        return;
    string str = string(luaL_checkstring(L, idx++)).lower();
    if (str == "energy")
        di.type = DT_Energy;
    else if (str == "kinetic")
        di.type = DT_Kinetic;
    else if (str == "emp")
        di.type = DT_EMP;
}
