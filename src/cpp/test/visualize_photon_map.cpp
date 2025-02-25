#include <omp.h>

#include "camera.hpp"
#include "image.hpp"
#include "integrator.hpp"
#include "photon_map.hpp"
#include "scene.hpp"

int main() {
    const int width = 512;
    const int height = 512;
    const int n_photons = 1000000;
    const int max_depth = 100;
    const int nb_thread = 8;

    Image image(width, height);

    float aspect_ratio = 16.0 / 9.0;
    Vec3f lookfrom(0, 1, 7);
    Vec3f lookat(0, 0, 0);
    Vec3f vup(0, 1, 0);
    float dist_to_focus = 10.0;
    float aperture = 0.1;
    Camera camera(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
    //Camera camera(Vec3f(0, 1, 7), Vec3f(0, 0, -1), 0.25 * PI);

    Scene scene;
    scene.loadModel("cornell_box.obj");
    scene.build();

    // photon tracing and build photon map
    PhotonMapping integrator(n_photons, 1,0, max_depth, nb_thread);
    UniformSampler sampler;
    integrator.build(scene, sampler);

    // visualize photon map
    std::cout << "[main] visualizing photon map" << std::endl;

    const PhotonMap photon_map = integrator.getPhotonMapGlobal();

#pragma omp parallel for collapse(2)
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            const float u = (2.0f * j - width) / height;
            const float v = (2.0f * i - height) / height;
            Ray ray;
            float pdf;
            if (camera.sampleRay(Vec2f(u, v), ray, pdf)) {
                IntersectInfo info;
                if (scene.intersect(ray, info)) {
                    // query photon map
                    float r2;
                    const std::vector<int> photon_indices =
                            photon_map.queryKNearestPhotons(info.surfaceInfo.position, 1,
                                                            r2);
                    const int photon_idx = photon_indices[0];

                    // if distance to the photon is small enough, write photon's
                    // throughput to the image
                    if (r2 < 0.001f) {
                        const Photon &photon = photon_map.getIthPhoton(photon_idx);
                        image.setPixel(i, j, photon.throughput);
                    }
                } else {
                    image.setPixel(i, j, Vec3fZero);
                }
            } else {
                image.setPixel(i, j, Vec3fZero);
            }
        }
    }

    image.writePPM("output.ppm");

    return 0;
}
