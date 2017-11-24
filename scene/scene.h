#ifndef _SCENE_H_
#define _SCENE_H_

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include "camera.h"
#include "sceneobject.h"
#include "rigidbody.h"
#include "light.h"
#include "settings/settings.h"

// Bullet physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"

/**
 * @class Scene
 * @brief Manages the update and rendering of all objects
 */
class Scene {
    public:
        /**
         * @brief Scene constructor
         * @details Creates a new scene, the world that holds all objects
         * and is also in charge of the physics (using bullets)
         * 
         * @param viewport_width Width in pixels of the viewport
         * @param viewport_height Height in pixels of the viewport
         */
        Scene(int viewport_width, int viewport_height);

        /* Destructor */
        ~Scene();

        /**
         * @brief Returns a reference to the scene's camera
         * @return A reference to the camera
         */
        Camera& camera();

        /**
         * @brief Adds a new object to the scene
         * @details Adds a new object to the scene. The object will be rendered
         * first if it is translucent.
         * 
         * @param object_id A unique name for the object
         * @param o A pointer to the object
         * @param is_translucent A bool that tells the scene if it is 
         *                       translucent
         */
        void add_object(const std::string& object_id,
                        std::shared_ptr<SceneObject> o,
                        bool is_translucent=false);

        /**
         * @brief Adds a new rigid body to the scene
         * @details Adds a new object to the scene. The object will be rendered
         * first if it is translucent. The object will first go through the 
         * physics pipeline before being rendered.
         * 
         * @param object_id A unique name for the object
         * @param o A pointer to the object
         * @param is_translucent A bool that tells the scene if it is 
         *                       translucent
         */
        void add_rigid_body(const std::string& object_id,
                            std::shared_ptr<RigidBody> o,
                            bool is_translucent=false);

        /**
         * @brief Adds a new directional light to the scene.
         * 
         * @param light The directional light to add
         */
        void add_directional_light(const DirectionalLight& light);

        /**
         * @brief Find an object in the scene by its id
         * 
         * @param object_id The id of the object
         * @return A shared ptr to the scene object, or nullptr
         */
        std::shared_ptr<SceneObject> get_object(const std::string& object_id);

        /**
         * @brief Renders the scene
         * 
         * @param dt Delta time to advance physics
         * @param dest_fbo Destination FBO to write the rendered scene
         */
        void render(float dt, GLuint dest_fbo);

        /**
         * @brief Loads a scene from a JSON file
         * 
         * @param filename JSON filename where the scene is described
         * @param viewport_width Viewport width
         * @param viewport_height Viewport height
         * @return A pointer to the newly created scene
         */
        static std::unique_ptr<Scene> load_scene(const std::string& filename, 
                                                 int viewport_width, 
                                                 int viewport_height);

        /**
         * @brief Resets ths scene
         * @details Resets the scene with the new configuration.
         * 
         * @param s_settings [description]
         * @param p_settings [description]
         * @param g_settings [description]
         */
        void reset(SimulationSettings s_settings,
                   PhysicsSettings p_settings,
                   GraphicsSettings g_settings);


    private:
        // A list that points the all translucent objects within the scene
        std::list<std::shared_ptr<SceneObject> > _solid_objects, _translucent_objects;
        
        // Structure to hold all renderable items on the scene
        std::map<std::string, std::shared_ptr<SceneObject> > _scene_objects;

        // The list of directional lights
        std::vector<DirectionalLight> _dir_lights;

        // A dict of initial position and rotation of each object
        std::unordered_map<std::string, btVector3> _initial_positions;
        std::unordered_map<std::string, btQuaternion> _initial_rotations;

        Camera _camera;

        int _viewport_w, _viewport_h;

        // The scene is rendered to a texture
        // when everything has been rendered, is blitted to the default FB
        GLuint _bkg_fbo;
        GLuint _bkg_color_tex;
        GLuint _bkg_depth_tex;

        // Skybox
        std::unique_ptr<QOpenGLShaderProgram> _shader;
        GLuint _skybox_vao;
        GLuint _skybox_vertices_vbo;
        GLuint _skybox_texture;

        // Helper functions
        void _initialize_fbo(int viewport_width, 
                             int viewport_height,
                             GLuint& fbo,
                             GLuint& color_tex,
                             GLuint& depth_tex);
        void _intialize_skybox();
        void _intialize_bullets();
        void _render_skybox();
        void _copy_fbo(GLuint from_fbo, GLuint dest_fbo); 

        // World rigid body physics
        btBroadphaseInterface* _bt_broadphase;
        btDefaultCollisionConfiguration* _bt_collision_configuration;
        btCollisionDispatcher* _bt_dispatcher;
        btSequentialImpulseConstraintSolver* _bt_solver;
        btDiscreteDynamicsWorld* _bt_world;

        btCollisionShape* _bt_ground_shape;
};

#endif // _SCENE_H_
